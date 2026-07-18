#pragma once
#include "../EffectModule.h"

/** Stereo-linked VCA compressor with soft knee. Implemented from scratch
    (not juce::dsp::Compressor) so we can expose a GR meter to the UI.

    Detection: peak with ballistic attack/release applied to the gain signal
    (not the level signal), which gives the classic "gain computer + smoother"
    topology used in most hardware compressors. Stereo-linked: both channels
    share a single gain reduction value derived from whichever is louder. */
class CompressorModule : public EffectModule
{
public:
    CompressorModule() : EffectModule ("comp") {}

    void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void attachToState (juce::AudioProcessorValueTreeState& apvts) override;
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    juce::String getDisplayName() const override { return "Compressor"; }

    /** Current gain reduction in dB (0 = none, negative = compressing). Thread-safe. */
    float getCurrentGainReductionDb() const { return grMeter.load (std::memory_order_relaxed); }

private:
    double sampleRate    = 44100.0;
    float  grSmoothed    = 0.0f;   // current smoothed gain reduction in dB
    float  attackCoeff   = 0.0f;
    float  releaseCoeff  = 0.0f;

    std::atomic<float> grMeter { 0.0f };

    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* ratioParam     = nullptr;
    std::atomic<float>* attackParam    = nullptr;
    std::atomic<float>* releaseParam   = nullptr;
    std::atomic<float>* kneeParam      = nullptr;
    std::atomic<float>* makeupParam    = nullptr;

    /** Computes the static gain-reduction (in dB, ≤ 0) for a given input level. */
    float staticCurve (float inputDb, float threshold, float ratio, float kneeDb) const noexcept;

    void updateCoefficients();
};
