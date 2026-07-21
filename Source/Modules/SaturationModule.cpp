#include "SaturationModule.h"

// ──────────────────────────────────────────── static waveshaper (shared with UI)

float SaturationModule::waveshapeStatic (float x, float drive, int mode) noexcept
{
    switch (mode)
    {
        case 0: // Tape — symmetric tanh, normalised so drive=1 → unity gain
        {
            float denom = std::tanh (drive);
            return denom > 1e-6f ? std::tanh (x * drive) / denom : x;
        }
        case 1: // Tube — asymmetric: positive half compressed harder
        {
            float denom = std::tanh (drive);
            if (denom < 1e-6f) return x;
            float driven = x * drive;
            return driven >= 0.0f
                ? std::tanh (driven) / denom
                : -std::tanh (-driven * 0.55f) / std::tanh (drive * 0.55f);
        }
        case 2: // Clip — hard limit; aliasing suppressed by 2x oversampling
            return juce::jlimit (-1.0f, 1.0f, x * drive);

        case 3: // Foldback — reflect signal at ±1 threshold
        {
            float v = x * drive;
            constexpr float thr = 1.0f;
            for (int iter = 0; iter < 8 && std::abs (v) > thr; ++iter)
            {
                if (v >  thr) v = 2.0f * thr - v;
                else          v = -2.0f * thr - v;
            }
            return v;
        }
        default:
            return x;
    }
}

// ──────────────────────────────────────────── parameters

void SaturationModule::addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    EffectModule::addParameters (layout);

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "sat_mode", 1 }, "Mode",
        juce::StringArray { "Tape", "Tube", "Clip", "Foldback" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sat_drive", 1 }, "Drive",
        juce::NormalisableRange<float> (1.0f, 20.0f, 0.01f, 0.4f), 2.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sat_tone", 1 }, "Tone (dB)",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sat_mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sat_output", 1 }, "Output (dB)",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
}

void SaturationModule::attachToState (juce::AudioProcessorValueTreeState& apvts)
{
    EffectModule::attachToState (apvts);
    driveParam  = apvts.getRawParameterValue ("sat_drive");
    modeParam   = apvts.getRawParameterValue ("sat_mode");
    toneParam   = apvts.getRawParameterValue ("sat_tone");
    mixParam    = apvts.getRawParameterValue ("sat_mix");
    outputParam = apvts.getRawParameterValue ("sat_output");
}

// ──────────────────────────────────────────── DSP lifecycle

void SaturationModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    // 2× oversampling, polyphase IIR half-band filter — reasonably cheap
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        spec.numChannels,
        1,  // factor = 2^1 = 2×
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true /* integer latency */);
    oversampling->initProcessing (spec.maximumBlockSize);

    juce::dsp::ProcessSpec mono { spec.sampleRate, spec.maximumBlockSize, 1 };
    auto flat = FilterCoefs::makeHighShelf (sampleRate, 4000.0f, 0.707f, 1.0f);
    toneFilterL.coefficients = flat;
    toneFilterR.coefficients = flat;
    toneFilterL.prepare (mono);
    toneFilterR.prepare (mono);
}

void SaturationModule::reset()
{
    if (oversampling) oversampling->reset();
    toneFilterL.reset();
    toneFilterR.reset();
}

// ──────────────────────────────────────────── process

void SaturationModule::process (juce::AudioBuffer<float>& buffer)
{
    auto drive  = driveParam  != nullptr ? driveParam->load()        : 2.0f;
    auto mode   = modeParam   != nullptr ? (int) modeParam->load()   : 0;
    auto tone   = toneParam   != nullptr ? toneParam->load()         : 0.0f;
    auto mix    = mixParam    != nullptr ? mixParam->load()          : 0.3f;
    auto outDb  = outputParam != nullptr ? outputParam->load()       : 0.0f;

    auto numSamples  = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    // Save dry for parallel mix (before oversampler touches the buffer)
    juce::AudioBuffer<float> dry (numChannels, numSamples);
    for (int ch = 0; ch < numChannels; ++ch)
        dry.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    // ── 2× upsample ──────────────────────────────────────────────────────
    juce::dsp::AudioBlock<float> block (buffer);
    auto upBlock = oversampling->processSamplesUp (block);
    auto upSamples = upBlock.getNumSamples();
    auto upCh      = upBlock.getNumChannels();

    // ── Waveshape ─────────────────────────────────────────────────────────
    for (size_t ch = 0; ch < upCh; ++ch)
    {
        auto* data = upBlock.getChannelPointer (ch);
        for (size_t s = 0; s < upSamples; ++s)
            data[s] = waveshapeStatic (data[s], drive, mode);
    }

    // ── 2× downsample ────────────────────────────────────────────────────
    oversampling->processSamplesDown (block);

    // ── Post-saturation tone shelf (high shelf at 4 kHz) ─────────────────
    auto toneGain = juce::Decibels::decibelsToGain (tone);
    auto toneCoefs = FilterCoefs::makeHighShelf (sampleRate, 4000.0f, 0.707f, toneGain);
    toneFilterL.coefficients = toneCoefs;
    toneFilterR.coefficients = toneCoefs;

    if (numChannels >= 1)
    {
        auto* ch0 = buffer.getWritePointer (0);
        for (int s = 0; s < numSamples; ++s)
            ch0[s] = toneFilterL.processSample (ch0[s]);
    }
    if (numChannels >= 2)
    {
        auto* ch1 = buffer.getWritePointer (1);
        for (int s = 0; s < numSamples; ++s)
            ch1[s] = toneFilterR.processSample (ch1[s]);
    }

    // ── Output trim + dry/wet mix ─────────────────────────────────────────
    auto outGain = juce::Decibels::decibelsToGain (outDb);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* wet = buffer.getWritePointer (ch);
        auto* d   = dry.getReadPointer (ch);
        for (int s = 0; s < numSamples; ++s)
            wet[s] = d[s] * (1.0f - mix) + wet[s] * outGain * mix;
    }
}
