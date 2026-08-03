#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../EffectModule.h"

/** Fixed pitch shift with independent formant tilt and dry/wet mix
    Uses a 4-tap overlap-add granular pitch shifter.

    Formant here is a spectral tilt (a low/high shelf pair around 800 Hz) —
    the same lightweight approximation Autotune's Formant knob uses */
class PitchFormantModule : public EffectModule
{
public:
    PitchFormantModule() : EffectModule ("pitchfx") {}

    void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void attachToState (juce::AudioProcessorValueTreeState& apvts) override;
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    juce::String getDisplayName() const override { return "Pitch/Formant"; }

    int getLatencySamples() const noexcept override
    {
        if (isBypassed()) return 0;
        return voiceL.grainSize;
    }

private:
    static constexpr float kGrainMs = 30.0f;   // fixed compromise between smoothness and latency
    /** Mono granular pitch shifter (4 overlapping taps at 25% spacing —
        raised-cosine-squared windows at that spacing sum to an exact
        constant, hence the 0.5 normalisation factor in the .cpp). */
    struct GrainVoice
    {
        std::vector<float> buf;
        int   bufSize = 0, grainSize = 0, maxGrain = 0;
        float writePos = 0;

        static constexpr int kNumTaps = 4;
        float readPos[kNumTaps] = { 0.0f, 0.0f, 0.0f, 0.0f };

        void prepare (double sampleRate, float grainMs);
        void resetBuffers();

        /** Feed one input sample at the given pitch ratio; returns the
            shifted output sample. Also writes delayedDry: the raw input
            sample from grainSize samples ago, time-aligned with this
            voice's output group delay, for phase-correct dry/wet mixing. */
        float processSample (float inSample, float ratio, float& delayedDry);

    private:
        static float readInterp (const std::vector<float>& b, float pos, int bsz);
        float dist (float rp) const;
    };

    GrainVoice voiceL, voiceR;
    juce::dsp::IIR::Filter<float> tiltLowL, tiltHighL, tiltLowR, tiltHighR;
    double sampleRate = 44100.0;

    std::atomic<float>* pitchParam   = nullptr;  // semitones, -12..+12
    std::atomic<float>* formantParam = nullptr;  // -1..+1
    std::atomic<float>* mixParam     = nullptr;  // 0-100 %
};
