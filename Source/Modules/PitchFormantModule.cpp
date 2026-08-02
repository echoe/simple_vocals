#include "PitchFormantModule.h"

// ─── GrainVoice ──────────────────────────────────────────────────────────────

void PitchFormantModule::GrainVoice::prepare (double sr, float grainMs)
{
    maxGrain  = (int) (sr * grainMs / 1000.0) + 1;
    grainSize = maxGrain;
    bufSize   = maxGrain * 4;
    buf.assign ((size_t) bufSize, 0.0f);
    resetBuffers();
}

void PitchFormantModule::GrainVoice::resetBuffers()
{
    std::fill (buf.begin(), buf.end(), 0.0f);
    writePos = 0.0f;
    for (int t = 0; t < kNumTaps; ++t)
        readPos[t] = std::fmod ((float) (bufSize - grainSize) + (float) grainSize * (float) t / (float) kNumTaps
                                 + (float) bufSize, (float) bufSize);
}

float PitchFormantModule::GrainVoice::readInterp (const std::vector<float>& b, float pos, int bsz)
{
    int i0 = (int) pos;
    int i1 = (i0 + 1) % bsz;
    float frac = pos - (float) i0;
    return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * frac;
}

float PitchFormantModule::GrainVoice::dist (float rp) const
{
    float d = writePos - rp;
    if (d < 0.0f) d += (float) bufSize;
    return d;
}

float PitchFormantModule::GrainVoice::processSample (float inSample, float ratio, float& delayedDry)
{
    if (bufSize == 0 || grainSize == 0) { delayedDry = inSample; return inSample; }

    int wi = (int) writePos % bufSize;
    buf[(size_t) wi] = inSample;

    // Aligned dry reference for external dry/wet mixing: grainSize samples
    // behind the write head, matching this voice's approximate output group
    // delay, so the caller can mix dry+wet without comb-filtering.
    int dryIdx = ((wi - grainSize) % bufSize + bufSize) % bufSize;
    delayedDry = buf[(size_t) dryIdx];

    auto window = [&] (float d) -> float
    {
        float c = juce::jlimit (0.0f, (float) grainSize, d);
        float s = std::sin (juce::MathConstants<float>::pi * c / (float) grainSize);
        return s * s;
    };

    float sum = 0.0f;
    for (int t = 0; t < kNumTaps; ++t)
    {
        float w = window (dist (readPos[t]));
        sum += readInterp (buf, readPos[t], bufSize) * w;
    }
    // 4 raised-cosine-squared taps at 25% spacing sum to an exact constant
    // of 2.0 (constant-overlap-add); scale by 0.5 to normalise.
    float out = sum * 0.5f;

    writePos = std::fmod (writePos + 1.0f, (float) bufSize);
    auto advance = [&] (float& rp)
    {
        rp = std::fmod (rp + ratio, (float) bufSize);
        if (rp < 0.0f) rp += (float) bufSize;
    };
    auto maybeReset = [&] (float& rp)
    {
        float d = dist (rp);
        if (d > (float) grainSize * 2.0f)
            rp = std::fmod (writePos - (float) grainSize + (float) bufSize * 2.0f, (float) bufSize);
        else if (d > (float) grainSize * 1.15f)
            rp = writePos;
    };
    for (int t = 0; t < kNumTaps; ++t)
    {
        advance (readPos[t]);
        maybeReset (readPos[t]);
    }

    return out;
}

// ─── PitchFormantModule ──────────────────────────────────────────────────────

void PitchFormantModule::addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    // Off by default: adds real latency/CPU while active, so it should be an
    // opt-in choice rather than silently engaged.
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { moduleId + "_enabled", 1 }, getDisplayName() + " Enabled", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pitchfx_semitones", 1 }, "Pitch",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("st")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pitchfx_formant", 1 }, "Formant",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pitchfx_mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
}

void PitchFormantModule::attachToState (juce::AudioProcessorValueTreeState& apvts)
{
    enabledParam = apvts.getRawParameterValue (moduleId + "_enabled");
    pitchParam   = apvts.getRawParameterValue ("pitchfx_semitones");
    formantParam = apvts.getRawParameterValue ("pitchfx_formant");
    mixParam     = apvts.getRawParameterValue ("pitchfx_mix");
}

void PitchFormantModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    voiceL.prepare (sampleRate, kGrainMs);
    voiceR.prepare (sampleRate, kGrainMs);
    tiltLowL.reset(); tiltHighL.reset();
    tiltLowR.reset(); tiltHighR.reset();
}

void PitchFormantModule::reset()
{
    voiceL.resetBuffers();
    voiceR.resetBuffers();
    tiltLowL.reset(); tiltHighL.reset();
    tiltLowR.reset(); tiltHighR.reset();
}

void PitchFormantModule::process (juce::AudioBuffer<float>& buffer)
{
    float semitones = pitchParam   ? pitchParam->load()   : 0.0f;
    float formant    = formantParam ? formantParam->load() : 0.0f;
    float mix        = mixParam     ? juce::jlimit (0.0f, 1.0f, mixParam->load() / 100.0f) : 1.0f;

    float ratio = std::pow (2.0f, semitones / 12.0f);

    // Formant tilt: the same lightweight low/high-shelf-around-800Hz
    // approach as Autotune's Formant knob — a brightness tilt, not a true
    // LPC-based formant correction, kept simple to stay low-artifact.
    float tiltDb = formant * 6.0f;
    auto lowCoefs = juce::dsp::IIR::Coefficients<float>::makeLowShelf  (sampleRate, 800.0f, 0.707f,
                        juce::Decibels::decibelsToGain (-tiltDb));
    auto hiCoefs  = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, 800.0f, 0.707f,
                        juce::Decibels::decibelsToGain (tiltDb));
    tiltLowL.coefficients  = lowCoefs;
    tiltHighL.coefficients = hiCoefs;
    tiltLowR.coefficients  = lowCoefs;
    tiltHighR.coefficients = hiCoefs;

    int numChannels = buffer.getNumChannels();
    int numSamples  = buffer.getNumSamples();
    if (numChannels == 0) return;

    auto* left  = buffer.getWritePointer (0);
    auto* right = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        float dryL = 0.0f;
        float shiftedL = voiceL.processSample (left[i], ratio, dryL);
        shiftedL = tiltLowL.processSample (shiftedL);
        shiftedL = tiltHighL.processSample (shiftedL);
        left[i] = dryL + (shiftedL - dryL) * mix;

        if (right != nullptr)
        {
            float dryR = 0.0f;
            float shiftedR = voiceR.processSample (right[i], ratio, dryR);
            shiftedR = tiltLowR.processSample (shiftedR);
            shiftedR = tiltHighR.processSample (shiftedR);
            right[i] = dryR + (shiftedR - dryR) * mix;
        }
    }
}
