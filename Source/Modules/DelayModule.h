#pragma once
#include "../EffectModule.h"

/** Stereo delay with optional ping-pong and feedback tone shaping.
    Ping-pong mode routes: left repeat → right, right repeat → left.
    A one-pole LP filter on the feedback path rolls off high frequencies
    each repeat, giving a natural tape-echo character. */
class DelayModule : public EffectModule
{
public:
    DelayModule() : EffectModule ("dly") {}

    void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void attachToState (juce::AudioProcessorValueTreeState& apvts) override;
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    juce::String getDisplayName() const override { return "Delay"; }

private:
    static constexpr float kMaxDelayMs = 1000.0f;

    std::vector<float> delayBufL, delayBufR;
    int   writeIdx = 0;
    int   bufSize  = 0;
    float feedbackStateL = 0.0f, feedbackStateR = 0.0f; // one-pole LP state

    double sampleRate = 44100.0;

    std::atomic<float>* timeParam     = nullptr;  // ms
    std::atomic<float>* feedbackParam = nullptr;  // 0-0.95
    std::atomic<float>* mixParam      = nullptr;  // 0-1
    std::atomic<float>* toneParam     = nullptr;  // LP cutoff 500-20000Hz
    std::atomic<float>* pingPongParam = nullptr;  // bool
};
