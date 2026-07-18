#include "ReverbModule.h"

void ReverbModule::addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    EffectModule::addParameters (layout);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "verb_size", 1 }, "Size",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.4f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "verb_damping", 1 }, "Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "verb_width", 1 }, "Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "verb_predelay", 1 }, "Pre-delay (ms)",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "verb_mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.2f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "verb_freeze", 1 }, "Freeze", false));
}

void ReverbModule::attachToState (juce::AudioProcessorValueTreeState& apvts)
{
    EffectModule::attachToState (apvts);

    sizeParam     = apvts.getRawParameterValue ("verb_size");
    dampingParam  = apvts.getRawParameterValue ("verb_damping");
    widthParam    = apvts.getRawParameterValue ("verb_width");
    predelayParam = apvts.getRawParameterValue ("verb_predelay");
    mixParam      = apvts.getRawParameterValue ("verb_mix");
    freezeParam   = apvts.getRawParameterValue ("verb_freeze");
}

void ReverbModule::updateReverbParams()
{
    juce::dsp::Reverb::Parameters p;
    p.roomSize   = sizeParam    != nullptr ? sizeParam->load()    : 0.4f;
    p.damping    = dampingParam != nullptr ? dampingParam->load() : 0.5f;
    p.width      = widthParam   != nullptr ? widthParam->load()   : 1.0f;
    p.freezeMode = freezeParam  != nullptr ? freezeParam->load()  : 0.0f;
    // Wet/dry is handled manually — keep engine fully wet.
    p.wetLevel   = 1.0f;
    p.dryLevel   = 0.0f;
    reverb.setParameters (p);
}

void ReverbModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    preDelaySamplesMax = (int) (sampleRate * 0.1) + 2; // 100 ms headroom
    preDelayBufL.assign ((size_t) preDelaySamplesMax, 0.0f);
    preDelayBufR.assign ((size_t) preDelaySamplesMax, 0.0f);
    preDelayWriteIdx = 0;

    reverb.prepare (spec);
    updateReverbParams();
}

void ReverbModule::process (juce::AudioBuffer<float>& buffer)
{
    updateReverbParams();

    auto numSamples  = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();
    auto mix         = mixParam != nullptr ? mixParam->load() : 0.2f;

    // ── Save dry signal ───────────────────────────────────────────────────
    juce::AudioBuffer<float> dry (numChannels, numSamples);
    for (int ch = 0; ch < numChannels; ++ch)
        dry.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    // ── Apply pre-delay to a scratch copy (wet path only) ─────────────────
    int delaySamples = 0;
    if (predelayParam != nullptr)
        delaySamples = juce::jlimit (0, preDelaySamplesMax - 1,
                                      (int) (predelayParam->load() / 1000.0f * (float) sampleRate));

    if (delaySamples > 0)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            int readIdx = (preDelayWriteIdx - delaySamples + preDelaySamplesMax)
                          % preDelaySamplesMax;

            if (numChannels >= 1)
            {
                preDelayBufL[(size_t) preDelayWriteIdx] = buffer.getSample (0, s);
                buffer.setSample (0, s, preDelayBufL[(size_t) readIdx]);
            }
            if (numChannels >= 2)
            {
                preDelayBufR[(size_t) preDelayWriteIdx] = buffer.getSample (1, s);
                buffer.setSample (1, s, preDelayBufR[(size_t) readIdx]);
            }

            preDelayWriteIdx = (preDelayWriteIdx + 1) % preDelaySamplesMax;
        }
    }

    // ── Run reverb engine (all wet) ───────────────────────────────────────
    juce::dsp::AudioBlock<float>            block  (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    reverb.process (ctx);

    // ── Manual dry/wet blend ──────────────────────────────────────────────
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* wet = buffer.getWritePointer (ch);
        auto* d   = dry.getReadPointer (ch);
        for (int s = 0; s < numSamples; ++s)
            wet[s] = d[s] * (1.0f - mix) + wet[s] * mix;
    }
}

void ReverbModule::reset()
{
    reverb.reset();

    std::fill (preDelayBufL.begin(), preDelayBufL.end(), 0.0f);
    std::fill (preDelayBufR.begin(), preDelayBufR.end(), 0.0f);
    preDelayWriteIdx = 0;
}
