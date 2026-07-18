#pragma once
#include "../EffectModule.h"

/** Split-band de-esser. The signal is split at `deess_freq` using complementary
    LP/HP filters. Only the high band is compressed; the low band passes through
    untouched and is recombined at the output, so the tonal character below the
    crossover is preserved.

    Detection: peak envelope follower on the HIGH band only (fast attack 2ms,
    slow release 80ms). When the envelope exceeds `deess_threshold`, gain
    reduction up to `deess_range` dB is applied to the high band. */
class DeEsserModule : public EffectModule
{
public:
    DeEsserModule() : EffectModule ("deess") {}

    void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void attachToState (juce::AudioProcessorValueTreeState& apvts) override;
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    juce::String getDisplayName() const override { return "De-Esser"; }

    /** Current gain reduction in dB (0 = none, negative = reducing). Thread-safe. */
    float getCurrentGainReductionDb() const { return grMeter.load (std::memory_order_relaxed); }

private:
    using Filter      = juce::dsp::IIR::Filter<float>;
    using FilterCoefs = juce::dsp::IIR::Coefficients<float>;

    // Per-channel split filters
    Filter lpL, lpR;  // low band  (passes through unchanged)
    Filter hpL, hpR;  // high band (compressed when sibilant)

    float  envelopeDb = -100.0f;
    double sampleRate = 44100.0;

    std::atomic<float> grMeter { 0.0f };

    std::atomic<float>* freqParam      = nullptr;  // crossover + detection frequency
    std::atomic<float>* thresholdParam = nullptr;  // dB level that triggers reduction
    std::atomic<float>* rangeParam     = nullptr;  // max gain reduction in dB
};
