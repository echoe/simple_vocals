#pragma once
#include "../EffectModule.h"

/** Two-voice granular pitch shifter with pitch humanisation and stereo width.

    Humanisation: each voice has an independent slow random pitch walk
    (±0..15 cents, updated every 0.3–1.5 s, smoothed over ~150 ms) that
    mimics the micro-intonation variations of a real singer.

    Stereo width (Haas effect): V1's R channel and V2's L channel receive a
    small additional delay (0–12 ms), making V1 lean left and V2 lean right
    in the stereo image — just like two singers panned to opposite sides. */
class HarmonizerModule : public EffectModule
{
public:
    HarmonizerModule() : EffectModule ("harm") {}

    void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void attachToState (juce::AudioProcessorValueTreeState& apvts) override;
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    juce::String getDisplayName() const override { return "Harmonizer"; }

    // Like Autotune, this is a granular pitch shifter: the read heads
    // necessarily trail the write head by roughly one grain length, so the
    // Grain (ms) knob is directly a latency control (30-150ms range, 80ms
    // default). Reported to the host via getLatencySamples() -> EffectChain
    // -> AudioProcessor::setLatencySamples() so the DAW can compensate.
    // (The Haas stereo-width delay is a per-channel creative effect, not
    // reported here — like the Delay module's echoes, it's not something a
    // whole-plugin output delay could meaningfully "compensate" for.)
    int getLatencySamples() const noexcept override
    {
        if (isBypassed()) return 0;
        return voice1.grainSize;   // voice1 and voice2 always share the same grain size
    }

private:
    // ── Grain voice ───────────────────────────────────────────────────────
    struct GrainVoice
    {
        std::vector<float> bufL, bufR;
        int   bufSize = 0, maxGrain = 0, grainSize = 0;
        float writePos = 0;

        // 4 evenly-spaced, overlapping read heads instead of 2. This is the
        // single biggest lever on perceived quality for a granular pitch
        // shifter: with only 2 taps, the two grains beat against each other
        // once per grain cycle, producing an audible ~6–15 Hz amplitude
        // "warble"/flutter on top of the pitch shift. Four taps at 25%
        // spacing quadruple the overlap density, which smooths that
        // modulation out almost entirely while keeping the same grain size
        // (and therefore the same latency/transient smearing trade-off).
        static constexpr int kNumTaps = 4;
        float readPos[kNumTaps] = { 0.0f, 0.0f, 0.0f, 0.0f };

        // Pitch humanisation state (independent per voice)
        float humTarget   = 0.0f;  // target pitch offset in cents
        float humSmoothed = 0.0f;  // current smoothed offset in cents
        int   humTimer    = 0;     // samples until next target change
        juce::Random humRng;       // seeded differently per voice

        void prepare (double sampleRate, float maxGrainMs, int rngSeed);
        void resetBuffers();
        void setGrainSize (double sampleRate, float grainMs);

        /** Pull humanisation toward a new random target. Call once per block. */
        void updateHuman (float maxCents, double sampleRate, int numSamples);

        void processSample (float inL, float inR,
                            float& outL, float& outR, float ratio);
    private:
        static float readInterp (const std::vector<float>& buf, float pos, int bsz);
        float dist (float rp) const;
    };

    // ── Haas stereo-width delay ───────────────────────────────────────────
    struct HaasDelay
    {
        std::vector<float> buf;
        int writeIdx = 0, bufSize = 0;

        void  prepare (double sampleRate);  // allocates for max 15 ms
        void  reset();
        float tick (float input, int delaySamples);
    };

    GrainVoice voice1, voice2;
    HaasDelay  haasV1, haasV2;   // one per voice (applied to the "far" channel)
    double     sampleRate = 44100.0;

    std::atomic<float>* interval1Param = nullptr;
    std::atomic<float>* interval2Param = nullptr;
    std::atomic<float>* mixParam       = nullptr;
    std::atomic<float>* grainParam     = nullptr;
    std::atomic<float>* humaniseParam  = nullptr;
    std::atomic<float>* widthParam     = nullptr;
};
