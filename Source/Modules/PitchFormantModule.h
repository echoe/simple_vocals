#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../EffectModule.h"

/** Fixed pitch shift with independent formant tilt and dry/wet mix — this
    is NOT pitch correction. Unlike Autotune, it never detects your pitch or
    tries to snap it toward a scale; it just transposes the whole signal by
    a fixed number of semitones you dial in, with an independent Formant
    control so the transposed voice doesn't automatically sound like a
    chipmunk (shifted up) or a giant (shifted down).

    Uses a 4-tap overlap-add granular pitch shifter (the highest-quality
    pitch-shift engine in this plugin), so expect
    similar artifact behaviour: clean for modest shifts (a few semitones),
    progressively more audible grain/flutter at larger ones. Like every
    other pitch-shifting module here, it has real processing latency
    (reported via getLatencySamples()) — and because that latency means the
    wet signal is time-shifted relative to a naive "current sample" dry
    signal, the dry reference used for the Mix knob is pulled from the same
    delay line the shifter reads from (see GrainVoice::processSample) so
    dry and wet stay phase-aligned instead of comb-filtering when blended.

    Formant here is a spectral tilt (a low/high shelf pair around 800 Hz) —
    the same lightweight approximation Autotune's Formant knob uses, not a
    true LPC-based formant-preserving algorithm. It's a solid, low-artifact
    way to warm up or thin out a shifted voice, but it won't perfectly hold
    exact vowel character the way dedicated formant-correction software
    does — worth knowing going in. */
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
    static constexpr float kGrainMs = 70.0f;   // fixed compromise between smoothness and latency

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
