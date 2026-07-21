#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include "../EffectModule.h"

/** Classical spectral-subtraction noise reduction ("denoise").

    Workflow: point the plugin at a few seconds of room tone / hiss with no
    vocal (hit "Learn Noise Profile"), then it captures an average magnitude
    spectrum of that noise. While active, every analysis frame's magnitude
    spectrum is compared against that learned profile; frequency bins where
    the incoming signal is close to the noise level get attenuated, bins
    clearly above the noise floor (i.e. the vocal) pass through mostly
    unaffected. A spectral floor prevents any bin from being fully muted,
    which is what avoids the "musical noise" (random tonal blips) that a
    naive full subtraction would produce.

    This is classical DSP (Boll-style spectral subtraction via STFT
    overlap-add), not a trained model — a solid, well-understood technique
    for steady background noise (hiss, hum, fans, room tone), not a cure for
    non-stationary noise (traffic, talking, clatter).

    Implementation note: like Autotune/Harmonizer, this module has genuine
    processing latency (reported via getLatencySamples()) — an STFT can't
    emit its first output sample until a full analysis window has been
    buffered. */
class NoiseReductionModule : public EffectModule
{
public:
    NoiseReductionModule() : EffectModule ("denoise") {}

    void addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void attachToState (juce::AudioProcessorValueTreeState& apvts) override;
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void process (juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    juce::String getDisplayName() const override { return "Denoise"; }

    static constexpr float kLearnSeconds = 1.5f;

    /** UI calls this to begin capturing a noise profile from whatever's
        coming into the module right now (point a mic at room tone first). */
    void startLearning() noexcept { learnRequested.store (true, std::memory_order_relaxed); }
    bool isLearning()  const noexcept { return learningActive.load (std::memory_order_relaxed); }
    bool hasProfile()  const noexcept { return profileLearned.load  (std::memory_order_relaxed); }

    int getLatencySamples() const noexcept override
    {
        if (isBypassed()) return 0;
        return kFFTSize;
    }

private:
    static constexpr int kFFTOrder = 10;
    static constexpr int kFFTSize  = 1 << kFFTOrder;   // 1024
    static constexpr int kHopSize  = kFFTSize / 2;      // 512, i.e. 50% overlap
    static constexpr int kNumBins  = kFFTSize / 2 + 1;

    /** One channel's full analysis -> spectral-subtract -> synthesis pipeline,
        using the standard JUCE circular-FIFO STFT pattern: a single ring
        buffer doubles as both the analysis window source and the overlap-add
        output accumulator, indexed by a shared rotating cursor. */
    struct STFTChannel
    {
        juce::dsp::FFT fft { kFFTOrder };
        std::vector<float> window;        // Hann, length kFFTSize; 50% hop -> exact constant-overlap-add
        std::vector<float> inputFifo;      // length kFFTSize, circular
        std::vector<float> outputFifo;     // length kFFTSize, circular, overlap-add accumulator
        std::vector<float> fftBuffer;      // length kFFTSize*2, scratch space for JUCE's real FFT

        int fifoIndex       = 0;   // shared read/write rotation cursor
        int samplesSinceHop = 0;

        std::array<float, kNumBins> noiseProfile {};
        std::array<float, kNumBins> noiseAccum   {};
        int  noiseFrameCount  = 0;
        int  noiseFrameTarget = 0;
        bool learning    = false;
        bool hasProfile_ = false;

        void prepare();
        void reset();
        void startLearning (double sampleRate, float learnSeconds);

        /** Feed one input sample, get one (delayed) output sample back.
            Sets justFinishedLearning = true on the sample where this
            channel's learning window completes. */
        float processSample (float inSample, float reductionAmt, float sensitivity,
                              bool& justFinishedLearning);

    private:
        void runFrame (float reductionAmt, float sensitivity, bool& justFinishedLearning);
    };

    std::array<STFTChannel, 2> channels;
    double sampleRate = 44100.0;

    std::atomic<bool> learnRequested { false };  // UI -> audio thread
    std::atomic<bool> learningActive { false };  // audio thread -> UI
    std::atomic<bool> profileLearned { false };  // audio thread -> UI

    std::atomic<float>* reductionParam   = nullptr;  // 0-100 %
    std::atomic<float>* sensitivityParam = nullptr;  // 0.5x - 3x, scales the learned profile before comparing
};
