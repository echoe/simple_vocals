#pragma once
#include "../EffectModule.h"

/** Freeverb-based stereo reverb with pre-delay.

    Signal chain:
      input → [pre-delay ring buffer] → juce::dsp::Reverb (wet only)
                                                                  ↘ + dry → output

    The dry/wet mix is handled manually so pre-delay only affects the
    reverb tail, not the dry signal. The Reverb engine's own dryLevel is
    always set to 0 and wetLevel to 1; we apply `verb_mix` ourselves. */
class ReverbModule : public EffectModule
{
public:
    ReverbModule() : EffectModule ("verb") {}

    void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void attachToState (juce::AudioProcessorValueTreeState& apvts) override;
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    juce::String getDisplayName() const override { return "Reverb"; }

private:
    juce::dsp::Reverb reverb;

    // Pre-delay ring buffers (one per channel, max 100 ms)
    std::vector<float> preDelayBufL, preDelayBufR;
    int   preDelayWriteIdx = 0;
    int   preDelaySamplesMax = 0;
    double sampleRate = 44100.0;

    std::atomic<float>* sizeParam      = nullptr;
    std::atomic<float>* dampingParam   = nullptr;
    std::atomic<float>* widthParam     = nullptr;
    std::atomic<float>* predelayParam  = nullptr;  // milliseconds
    std::atomic<float>* mixParam       = nullptr;
    std::atomic<float>* freezeParam    = nullptr;

    void updateReverbParams();
};
