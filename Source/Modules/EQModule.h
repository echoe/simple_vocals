#pragma once
#include "../EffectModule.h"

/** Up to 8 parametric bell bands. Each band has its own `enabled` toggle so
    any individual band can be added or removed independently. Default: 4 enabled.

    Uses one juce::dsp::IIR::Filter<float> per channel per band — avoids the
    ProcessorDuplicator shared-state-pointer pitfall where coefficient updates
    written to `state` after prepare() are invisible to the per-channel filters. */
class EQModule : public EffectModule
{
public:
    explicit EQModule (juce::String idIn = "eq", juce::String displayNameIn = "EQ")
        : EffectModule (std::move (idIn)), displayName (std::move (displayNameIn)) {}

    static constexpr int maxBands = 8;

    void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void attachToState (juce::AudioProcessorValueTreeState& apvts) override;
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    juce::String getDisplayName() const override { return displayName; }

    /** Band parameter ID for a given instance prefix (e.g. "eq" or "eq2"),
        so UI components can bind to either EQ instance's bands. */
    static juce::String bandParamID (const juce::String& idPrefix, int bandIndex, const char* suffix)
    {
        return idPrefix + "_band" + juce::String (bandIndex) + "_" + suffix;
    }

    /** Convenience overload for use inside this instance (uses its own id). */
    juce::String bandParamID (int bandIndex, const char* suffix) const
    {
        return bandParamID (moduleId, bandIndex, suffix);
    }

private:
    juce::String displayName;

    using Filter      = juce::dsp::IIR::Filter<float>;
    using FilterCoefs = juce::dsp::IIR::Coefficients<float>;

    struct Band
    {
        Filter filterL, filterR;       // per-channel processing filters
        Filter detectL, detectR;       // bandpass filters for dynamic-mode sidechain
        float  envelopeDb = -100.0f;

        std::atomic<float>* enabledParam    = nullptr;
        std::atomic<float>* freqParam       = nullptr;
        std::atomic<float>* gainParam       = nullptr;
        std::atomic<float>* qParam          = nullptr;
        std::atomic<float>* dynamicParam    = nullptr;
        std::atomic<float>* thresholdParam  = nullptr;
        std::atomic<float>* ratioParam      = nullptr;
    };

    std::array<Band, maxBands> bands;
    double sampleRate = 44100.0;

    void  updateBandCoefficients (Band& band, float freq, float q, float gainLinear);
    float computeTargetGainDb    (Band& band, const juce::AudioBuffer<float>& buffer);
};
