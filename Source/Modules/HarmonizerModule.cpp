#include "HarmonizerModule.h"

// ──────────────────────────────────────────────── GrainVoice

float HarmonizerModule::GrainVoice::readInterp (const std::vector<float>& b,
                                                  float p, int sz)
{
    int i0 = (int) std::floor (p) % sz;
    if (i0 < 0) i0 += sz;
    float frac = p - std::floor (p);
    return b[(size_t)i0] * (1.0f - frac) + b[(size_t)((i0+1)%sz)] * frac;
}

float HarmonizerModule::GrainVoice::dist (float rp) const
{
    float d = writePos - rp;
    while (d <  0.0f)           d += (float)bufSize;
    while (d >= (float)bufSize) d -= (float)bufSize;
    return d;
}

void HarmonizerModule::GrainVoice::prepare (double sr, float maxGrainMs, int rngSeed)
{
    maxGrain  = (int)(sr * maxGrainMs / 1000.0) + 1;
    grainSize = maxGrain;
    bufSize   = maxGrain * 4;
    bufL.assign ((size_t)bufSize, 0.0f);
    bufR.assign ((size_t)bufSize, 0.0f);
    humRng.setSeed (rngSeed);
    humTimer   = (int)(sr * 0.5);
    humTarget  = humSmoothed = 0.0f;
    resetBuffers();
}

void HarmonizerModule::GrainVoice::resetBuffers()
{
    std::fill (bufL.begin(), bufL.end(), 0.0f);
    std::fill (bufR.begin(), bufR.end(), 0.0f);
    writePos = 0.0f;
    for (int t = 0; t < kNumTaps; ++t)
        readPos[t] = std::fmod ((float)(bufSize - grainSize) + (float)grainSize * (float)t / (float)kNumTaps
                                 + (float)bufSize, (float)bufSize);
    humSmoothed = humTarget = 0.0f;
}

void HarmonizerModule::GrainVoice::setGrainSize (double sr, float grainMs)
{
    int newSize = juce::jlimit (1, maxGrain, (int)(sr * grainMs / 1000.0));
    if (newSize != grainSize)
    {
        grainSize = newSize;
        for (int t = 0; t < kNumTaps; ++t)
            readPos[t] = std::fmod (writePos - (float)grainSize
                                     + (float)grainSize * (float)t / (float)kNumTaps
                                     + (float)bufSize, (float)bufSize);
    }
}

void HarmonizerModule::GrainVoice::updateHuman (float maxCents, double sr,
                                                  int numSamples)
{
    humTimer -= numSamples;
    if (humTimer <= 0)
    {
        // New random target every 0.3–1.5 s, each voice independent
        humTimer  = (int)(sr * (0.3 + humRng.nextDouble() * 1.2));
        humTarget = (float)(humRng.nextDouble() * 2.0 - 1.0) * maxCents;
    }
    // One-pole smooth toward target: τ ≈ 150 ms applied per block
    float alpha = 1.0f - std::exp (-(float)numSamples / (float)(sr * 0.15));
    humSmoothed += (humTarget - humSmoothed) * alpha;
}

void HarmonizerModule::GrainVoice::processSample (float  inL, float  inR,
                                                    float& outL, float& outR,
                                                    float  ratio)
{
    if (bufSize == 0 || grainSize == 0) { outL = outR = 0.0f; return; }

    int wi = (int)writePos % bufSize;
    bufL[(size_t)wi] = inL;
    bufR[(size_t)wi] = inR;

    auto window = [&](float d) -> float {
        float c = juce::jlimit (0.0f, (float)grainSize, d);
        float s = std::sin (juce::MathConstants<float>::pi * c / (float)grainSize);
        return s * s;
    };

    float sumL = 0.0f, sumR = 0.0f;
    for (int t = 0; t < kNumTaps; ++t)
    {
        float w = window (dist (readPos[t]));
        sumL += readInterp (bufL, readPos[t], bufSize) * w;
        sumR += readInterp (bufR, readPos[t], bufSize) * w;
    }
    // 4 raised-cosine-squared taps spaced at 25% of the grain period sum to
    // an exact constant of 2.0 (constant-overlap-add), so scale by 0.5 to
    // keep the same output level as the previous 2-tap version.
    outL = sumL * 0.5f;
    outR = sumR * 0.5f;

    writePos = std::fmod (writePos + 1.0f, (float)bufSize);
    auto advance = [&](float& rp) {
        rp = std::fmod (rp + ratio, (float)bufSize);
        if (rp < 0.0f) rp += (float)bufSize;
    };

    auto maybeReset = [&](float& rp) {
        float d = dist (rp);
        if (d > (float)grainSize * 2.0f)
            rp = std::fmod (writePos - (float)grainSize + (float)bufSize * 2.0f, (float)bufSize);
        else if (d > (float)grainSize * 1.15f)
            rp = writePos;
    };
    for (int t = 0; t < kNumTaps; ++t)
    {
        advance (readPos[t]);
        maybeReset (readPos[t]);
    }
}

// ──────────────────────────────────────────────── HaasDelay

void HarmonizerModule::HaasDelay::prepare (double sr)
{
    bufSize = (int)(sr * 0.015) + 2;  // 15 ms headroom
    buf.assign ((size_t)bufSize, 0.0f);
    writeIdx = 0;
}

void HarmonizerModule::HaasDelay::reset()
{
    std::fill (buf.begin(), buf.end(), 0.0f);
    writeIdx = 0;
}

float HarmonizerModule::HaasDelay::tick (float input, int delaySamples)
{
    buf[(size_t)writeIdx] = input;
    int readIdx = (writeIdx - juce::jlimit (0, bufSize - 1, delaySamples) + bufSize) % bufSize;
    writeIdx = (writeIdx + 1) % bufSize;
    return buf[(size_t)readIdx];
}

// ──────────────────────────────────────────────── parameters

void HarmonizerModule::addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    EffectModule::addParameters (layout);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "harm_interval1", 1 }, "Voice 1 (st)",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 1.0f), 4.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "harm_interval2", 1 }, "Voice 2 (st)",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 1.0f), 7.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "harm_mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.4f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "harm_grain", 1 }, "Grain (ms)",
        juce::NormalisableRange<float> (30.0f, 150.0f, 1.0f), 80.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "harm_humanise", 1 }, "Humanise",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "harm_width", 1 }, "Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
}

void HarmonizerModule::attachToState (juce::AudioProcessorValueTreeState& apvts)
{
    EffectModule::attachToState (apvts);
    interval1Param = apvts.getRawParameterValue ("harm_interval1");
    interval2Param = apvts.getRawParameterValue ("harm_interval2");
    mixParam       = apvts.getRawParameterValue ("harm_mix");
    grainParam     = apvts.getRawParameterValue ("harm_grain");
    humaniseParam  = apvts.getRawParameterValue ("harm_humanise");
    widthParam     = apvts.getRawParameterValue ("harm_width");
}

// ──────────────────────────────────────────────── lifecycle

void HarmonizerModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    voice1.prepare (sampleRate, 150.0f, 12345);
    voice2.prepare (sampleRate, 150.0f, 67890);  // different seed → independent variation
    haasV1.prepare (sampleRate);
    haasV2.prepare (sampleRate);
}

void HarmonizerModule::reset()
{
    voice1.resetBuffers();
    voice2.resetBuffers();
    haasV1.reset();
    haasV2.reset();
}

// ──────────────────────────────────────────────── process

void HarmonizerModule::process (juce::AudioBuffer<float>& buffer)
{
    auto st1      = interval1Param ? interval1Param->load() : 4.0f;
    auto st2      = interval2Param ? interval2Param->load() : 7.0f;
    auto mix      = mixParam       ? mixParam->load()       : 0.4f;
    auto grainMs  = grainParam     ? grainParam->load()     : 80.0f;
    auto humanise = humaniseParam  ? humaniseParam->load()  : 0.3f;
    auto width    = widthParam     ? widthParam->load()     : 0.5f;

    auto numSamples  = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    // Update humanisation (once per block, per voice independently)
    float maxCents = humanise * 15.0f;   // up to ±15 cents at humanise=1
    voice1.updateHuman (maxCents, sampleRate, numSamples);
    voice2.updateHuman (maxCents, sampleRate, numSamples);

    // Pitch ratios — include per-voice humanisation offset
    float ratio1 = std::pow (2.0f, (st1 + voice1.humSmoothed / 100.0f) / 12.0f);
    float ratio2 = std::pow (2.0f, (st2 + voice2.humSmoothed / 100.0f) / 12.0f);

    voice1.setGrainSize (sampleRate, grainMs);
    voice2.setGrainSize (sampleRate, grainMs);

    // Haas delay: V1 delays its R channel, V2 delays its L channel
    // → V1 leans left, V2 leans right in the stereo image
    int haasSamples = juce::jlimit (0, haasV1.bufSize - 1,
                                     (int)(width * 0.012 * sampleRate));

    float harmGain = mix * 0.5f;

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = numChannels >= 1 ? buffer.getSample (0, s) : 0.0f;
        float inR = numChannels >= 2 ? buffer.getSample (1, s) : inL;

        float v1L, v1R, v2L, v2R;
        voice1.processSample (inL, inR, v1L, v1R, ratio1);
        voice2.processSample (inL, inR, v2L, v2R, ratio2);

        // Haas spread: delay the "far" channel of each voice
        v1R = haasV1.tick (v1R, haasSamples);   // V1 R delayed → V1 leans left
        v2L = haasV2.tick (v2L, haasSamples);   // V2 L delayed → V2 leans right

        if (numChannels >= 1)
            buffer.setSample (0, s, inL + (v1L + v2L) * harmGain);
        if (numChannels >= 2)
            buffer.setSample (1, s, inR + (v1R + v2R) * harmGain);
    }
}
