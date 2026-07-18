#pragma once
#include "../EffectModule.h"

/** Waveshaping saturation with four modes and 2x oversampling.

    Modes
    -----
    0  Tape      — tanh (symmetric soft clip, even + odd harmonics)
    1  Tube      — asymmetric tanh (richer even harmonics, warmer)
    2  Clip      — hard limiter at ±1 (bright, punchy; benefits most from OS)
    3  Foldback  — wavefolding (complex harmonic stack, metallic at high drive)

    Chain: [dry copy] → 2x upsample → waveshape → 2x downsample
           → post-tone shelf → output trim → dry/wet mix */
class SaturationModule : public EffectModule
{
public:
    SaturationModule() : EffectModule ("sat") {}

    static constexpr int numModes = 4;

    void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void attachToState (juce::AudioProcessorValueTreeState& apvts) override;
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    juce::String getDisplayName() const override { return "Saturation"; }

    /** Evaluate the static waveshaper (no drive normalisation) — used by the
        UI panel to draw the transfer curve without allocating audio buffers. */
    static float waveshapeStatic (float x, float drive, int mode) noexcept;

private:
    using Filter      = juce::dsp::IIR::Filter<float>;
    using FilterCoefs = juce::dsp::IIR::Coefficients<float>;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    Filter toneFilterL, toneFilterR;
    double sampleRate = 44100.0;

    std::atomic<float>* driveParam  = nullptr;
    std::atomic<float>* modeParam   = nullptr;
    std::atomic<float>* toneParam   = nullptr;
    std::atomic<float>* mixParam    = nullptr;
    std::atomic<float>* outputParam = nullptr;
};
