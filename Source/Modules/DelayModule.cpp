#include "DelayModule.h"

void DelayModule::addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    EffectModule::addParameters (layout);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "dly_time", 1 }, "Time (ms)",
        juce::NormalisableRange<float> (1.0f, kMaxDelayMs, 1.0f, 0.4f), 250.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "dly_feedback", 1 }, "Feedback",
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.35f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "dly_mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "dly_tone", 1 }, "Tone (Hz)",
        juce::NormalisableRange<float> (500.0f, 20000.0f, 1.0f, 0.3f), 6000.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "dly_pingpong", 1 }, "Ping-Pong", false));
}

void DelayModule::attachToState (juce::AudioProcessorValueTreeState& apvts)
{
    EffectModule::attachToState (apvts);
    timeParam     = apvts.getRawParameterValue ("dly_time");
    feedbackParam = apvts.getRawParameterValue ("dly_feedback");
    mixParam      = apvts.getRawParameterValue ("dly_mix");
    toneParam     = apvts.getRawParameterValue ("dly_tone");
    pingPongParam = apvts.getRawParameterValue ("dly_pingpong");
}

void DelayModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    bufSize    = (int) (sampleRate * kMaxDelayMs / 1000.0) + 2;
    delayBufL.assign ((size_t) bufSize, 0.0f);
    delayBufR.assign ((size_t) bufSize, 0.0f);
    writeIdx = 0;
    feedbackStateL = feedbackStateR = 0.0f;
}

void DelayModule::process (juce::AudioBuffer<float>& buffer)
{
    auto timeMs    = timeParam     != nullptr ? timeParam->load()     : 250.0f;
    auto feedback  = feedbackParam != nullptr ? feedbackParam->load() : 0.35f;
    auto mix       = mixParam      != nullptr ? mixParam->load()      : 0.3f;
    auto toneHz    = toneParam     != nullptr ? toneParam->load()     : 6000.0f;
    bool pingPong  = pingPongParam != nullptr && pingPongParam->load() > 0.5f;

    int delaySamples = juce::jlimit (1, bufSize - 1,
                                      (int) (timeMs / 1000.0f * (float) sampleRate));

    // One-pole LP coefficient for feedback tone
    float lpCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi
                                     * toneHz / (float) sampleRate);

    auto numSamples  = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = numChannels >= 1 ? buffer.getSample (0, s) : 0.0f;
        float inR = numChannels >= 2 ? buffer.getSample (1, s) : inL;

        // Read from delay buffer
        int readIdx = (writeIdx - delaySamples + bufSize) % bufSize;
        float delL = delayBufL[(size_t) readIdx];
        float delR = delayBufR[(size_t) readIdx];

        // Apply LP tone filter to feedback signal
        feedbackStateL += lpCoeff * (delL - feedbackStateL);
        feedbackStateR += lpCoeff * (delR - feedbackStateR);
        float fbL = feedbackStateL * feedback;
        float fbR = feedbackStateR * feedback;

        // Write: input + feedback (ping-pong swaps channels on feedback)
        delayBufL[(size_t) writeIdx] = pingPong ? inL + fbR : inL + fbL;
        delayBufR[(size_t) writeIdx] = pingPong ? inR + fbL : inR + fbR;
        writeIdx = (writeIdx + 1) % bufSize;

        // Output: dry + wet mix
        if (numChannels >= 1) buffer.setSample (0, s, inL * (1.0f - mix) + delL * mix);
        if (numChannels >= 2) buffer.setSample (1, s, inR * (1.0f - mix) + delR * mix);
    }
}

void DelayModule::reset()
{
    std::fill (delayBufL.begin(), delayBufL.end(), 0.0f);
    std::fill (delayBufR.begin(), delayBufR.end(), 0.0f);
    writeIdx = 0;
    feedbackStateL = feedbackStateR = 0.0f;
}
