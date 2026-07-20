#pragma once
#include "../EffectModule.h"

/** Monophonic pitch correction.

    Pipeline per block:
      1. Fill analysis ring buffer from (mono sum of) input.
      2. Every kHopSize samples: run autocorrelation pitch detector on the
         most recent kAnalysisN samples.
      3. Quantise detected pitch to the nearest note in the chosen key/scale.
      4. Smooth correction amount with the Retune Speed envelope.
      5. Pitch-shift the full stereo buffer by the correction ratio using
         a two-head Hann² granular shifter (same algorithm as Harmonizer).
      6. Blend dry + corrected by the Amount parameter.

    Exposes detectedHz / targetHz / correctionSt atomics for the UI. */
class AutotuneModule : public EffectModule
{
public:
    AutotuneModule() : EffectModule ("auto") {}

    void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void attachToState (juce::AudioProcessorValueTreeState& apvts) override;
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    juce::String getDisplayName() const override { return "Autotune"; }

    float getDetectedHz()  const noexcept { return detectedHz.load  (std::memory_order_relaxed); }
    float getTargetHz()    const noexcept { return targetHz.load    (std::memory_order_relaxed); }
    float getCorrectionSt()const noexcept { return correctionSt.load(std::memory_order_relaxed); }
    float getConfidence()  const noexcept { return confidence.load  (std::memory_order_relaxed); }

    // Latency mode: the granular pitch-shifter's grain size is the dominant
    // source of this module's processing latency (its read head necessarily
    // trails the write head by roughly one grain length). Live uses a short
    // grain for near-real-time monitoring at some cost to smoothness; Studio
    // uses the original, smoother, higher-latency grain. Latency is reported
    // to the host via EffectModule::getLatencySamples() -> EffectChain ->
    // AudioProcessor::setLatencySamples(), so the DAW can compensate.
    static constexpr float kLiveGrainMs   = 10.0f;
    static constexpr float kStudioGrainMs = 120.0f;

    int getLatencySamples() const noexcept override
    {
        if (isBypassed()) return 0;
        return currentLatencyMode == 0 ? corrVoiceLive.grainSize : corrVoiceStudio.grainSize;
    }

private:
    // ── Granular pitch-shift voice (same algorithm as HarmonizerModule) ──
    struct GrainVoice
    {
        std::vector<float> bufL, bufR;
        int   bufSize = 0, grainSize = 0, maxGrain = 0;
        float writePos = 0, readPos1 = 0, readPos2 = 0;

        void prepare (double sr, float maxGrainMs);
        void resetBuffers();
        void processSample (float inL, float inR, float& outL, float& outR, float ratio);

    private:
        static float lerp (const std::vector<float>& b, float p, int sz);
        float dist (float rp) const;
    };

    // ── Pitch detection ───────────────────────────────────────────────────
    static constexpr int kAnalysisN = 2048;
    static constexpr int kHopSize   = 256;

    std::vector<float> analysisBuf;
    int analysisWriteIdx = 0;
    int hopCounter       = 0;

    float smoothedCorrSt = 0.0f;
    double sampleRate    = 44100.0;

    std::atomic<float> detectedHz   { 0.0f };
    std::atomic<float> targetHz     { 0.0f };
    std::atomic<float> correctionSt { 0.0f };
    std::atomic<float> confidence   { 0.0f };

    GrainVoice corrVoiceLive;      // short grain: low latency, monitoring-friendly
    GrainVoice corrVoiceStudio;    // long grain: original smoothness/latency
    int currentLatencyMode = 1;    // 0 = Live, 1 = Studio — mirrors auto_latency_mode's default

    // ── Parameters ────────────────────────────────────────────────────────
    std::atomic<float>* speedParam     = nullptr;
    std::atomic<float>* amountParam    = nullptr;
    std::atomic<float>* mixParam       = nullptr;
    std::atomic<float>* keyParam       = nullptr;
    std::atomic<float>* formantParam   = nullptr;
    std::atomic<float>* characterParam = nullptr;   // 0=Natural, 1=Robotic
    std::atomic<float>* latencyModeParam = nullptr; // 0=Live, 1=Studio
    std::array<std::atomic<float>*, 12> noteParams { nullptr };

    // Formant-shift: complementary shelf filters applied after pitch correction
    using Filter      = juce::dsp::IIR::Filter<float>;
    using FilterCoefs = juce::dsp::IIR::Coefficients<float>;
    Filter formantLowL,  formantLowR;   // low shelf
    Filter formantHighL, formantHighR;  // high shelf

    // ── Helpers ───────────────────────────────────────────────────────────
    void  runPitchDetection();
    float quantisePitch (float hz) const;   // uses noteParams directly
};
