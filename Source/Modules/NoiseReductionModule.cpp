#include "NoiseReductionModule.h"

// ─── STFTChannel ─────────────────────────────────────────────────────────────

void NoiseReductionModule::STFTChannel::prepare()
{
    window.assign (kFFTSize, 0.0f);
    for (int i = 0; i < kFFTSize; ++i)
        window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) kFFTSize);

    inputFifo.assign  (kFFTSize, 0.0f);
    outputFifo.assign (kFFTSize, 0.0f);
    fftBuffer.assign  (kFFTSize * 2, 0.0f);

    fifoIndex = 0;
    samplesSinceHop = 0;
    noiseProfile.fill (0.0f);
    noiseAccum.fill (0.0f);
    smoothedGain.fill (1.0f);
    noiseRefLevel = 1.0f;
    noiseFrameCount = 0;
    learning = false;
    hasProfile_ = false;
}

void NoiseReductionModule::STFTChannel::reset()
{
    std::fill (inputFifo.begin(),  inputFifo.end(),  0.0f);
    std::fill (outputFifo.begin(), outputFifo.end(), 0.0f);
    fifoIndex = 0;
    samplesSinceHop = 0;
}

void NoiseReductionModule::STFTChannel::startLearning (double sr, float learnSeconds)
{
    noiseAccum.fill (0.0f);
    noiseFrameCount  = 0;
    noiseFrameTarget = juce::jmax (1, (int) ((sr * (double) learnSeconds) / (double) kHopSize));
    learning = true;
}

float NoiseReductionModule::STFTChannel::processSample (float inSample, float reductionAmt,
                                                          float sensitivity, int mode,
                                                          bool& justFinishedLearning)
{
    // Write the new sample into the slot that's currently the "oldest" (it
    // was consumed into a past analysis frame already, so it's safe to
    // overwrite). The output FIFO at that same slot holds a fully-summed,
    // ready-to-emit sample from the overlap-add of previous frames.
    inputFifo[(size_t) fifoIndex] = inSample;

    float outSample = outputFifo[(size_t) fifoIndex];
    outputFifo[(size_t) fifoIndex] = 0.0f;   // consumed — clear so future overlap-adds start clean

    fifoIndex = (fifoIndex + 1) % kFFTSize;
    ++samplesSinceHop;

    if (samplesSinceHop >= kHopSize)
    {
        samplesSinceHop -= kHopSize;
        runFrame (reductionAmt, sensitivity, mode, justFinishedLearning);
    }

    return outSample;
}

void NoiseReductionModule::STFTChannel::runFrame (float reductionAmt, float sensitivity,
                                                    int mode, bool& justFinishedLearning)
{
    // Pull the last kFFTSize samples out of the input ring in chronological
    // order (fifoIndex currently points at the oldest sample) and window them.
    for (int i = 0; i < kFFTSize; ++i)
    {
        int idx = (fifoIndex + i) % kFFTSize;
        fftBuffer[(size_t) i] = inputFifo[(size_t) idx] * window[(size_t) i];
    }
    std::fill (fftBuffer.begin() + kFFTSize, fftBuffer.end(), 0.0f);

    fft.performRealOnlyForwardTransform (fftBuffer.data());

    for (int k = 0; k < kNumBins; ++k)
    {
        float re  = fftBuffer[(size_t) (2 * k)];
        float im  = fftBuffer[(size_t) (2 * k + 1)];
        float mag = std::sqrt (re * re + im * im);

        float targetGain = 1.0f;   // clean passthrough unless we have a profile to reduce against

        if (learning)
        {
            noiseAccum[(size_t) k] += mag;
        }
        else if (hasProfile_ && mode == 0)
        {
            // Static: a fixed attenuation curve computed purely from the
            // learned profile — bins where the noise was more prominent
            // (relative to the loudest noise bin) get cut more, applied the
            // same way on every frame regardless of what's currently
            // playing. Because this never looks at the current frame's
            // content, pumping is structurally impossible here.
            constexpr float floorRatio = 0.08f;   // -22 dB floor, same reasoning as Adaptive below
            float normalisedNoise = noiseRefLevel > 1.0e-8f ? noiseProfile[(size_t) k] / noiseRefLevel : 0.0f;
            float cut = reductionAmt * normalisedNoise * sensitivity;
            targetGain = juce::jlimit (floorRatio, 1.0f, 1.0f - cut);
        }
        else if (hasProfile_ && mag > 1.0e-8f)
        {
            // Adaptive: compare this frame's magnitude against the profile
            // live. Bins near the noise threshold flicker between ~0 and
            // full level frame-to-frame if fully subtracted, heard as
            // "musical noise" — random tonal artifacts — hence the same
            // spectral floor as Static, plus the smoothing below.
            constexpr float floorRatio = 0.08f;

            float noiseMag   = noiseProfile[(size_t) k] * sensitivity;
            float subtracted = mag - noiseMag;
            float floor      = mag * floorRatio;
            float targetMag  = juce::jmax (subtracted, floor);
            float finalMag   = mag + (targetMag - mag) * reductionAmt;
            targetGain = finalMag / mag;
        }

        // Smooth the per-bin gain across frames rather than applying it raw.
        // In Static mode targetGain barely moves frame-to-frame anyway (only
        // when Reduction/Sensitivity are being turned, or mode/profile just
        // changed), so this mainly guards against zipper noise on those
        // knob moves. In Adaptive mode it's doing more work: a vocal's
        // spectral content shifts constantly syllable-to-syllable, and
        // without smoothing the reduction gain can swing abruptly frame-to-
        // frame, audible as the whole effect pumping/gating in and out.
        // Fast attack (reduce quickly) but slower release (recover
        // gradually) mirrors how a noise gate's envelope behaves.
        constexpr float attackCoeff  = 0.6f;   // ~2 frames (~23ms) to reach a lower target
        constexpr float releaseCoeff = 0.15f;  // ~7 frames (~80ms) to recover back up
        float& g = smoothedGain[(size_t) k];
        g += (targetGain - g) * (targetGain < g ? attackCoeff : releaseCoeff);

        fftBuffer[(size_t) (2 * k)]     = re * g;
        fftBuffer[(size_t) (2 * k + 1)] = im * g;
    }

    if (learning)
    {
        ++noiseFrameCount;
        if (noiseFrameCount >= noiseFrameTarget)
        {
            float maxProfile = 0.0f;
            for (int k = 0; k < kNumBins; ++k)
            {
                noiseProfile[(size_t) k] = noiseAccum[(size_t) k] / (float) noiseFrameCount;
                maxProfile = juce::jmax (maxProfile, noiseProfile[(size_t) k]);
            }
            noiseRefLevel = juce::jmax (1.0e-8f, maxProfile);
            learning = false;
            hasProfile_ = true;
            justFinishedLearning = true;
        }
    }

    fft.performRealOnlyInverseTransform (fftBuffer.data());

    // Overlap-add the reconstructed frame into the output ring, starting at
    // the same rotating cursor used for analysis.
    for (int i = 0; i < kFFTSize; ++i)
    {
        int idx = (fifoIndex + i) % kFFTSize;
        outputFifo[(size_t) idx] += fftBuffer[(size_t) i];
    }
}

// ─── NoiseReductionModule ────────────────────────────────────────────────────

void NoiseReductionModule::addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    // Overrides the base class's default-enabled-true behaviour: this module
    // does nothing useful without a learned profile, and it adds real
    // latency + CPU cost while active, so it should start off.
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { moduleId + "_enabled", 1 }, getDisplayName() + " Enabled", false));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "denoise_mode", 1 }, "Mode",
        juce::StringArray { "Static", "Adaptive" }, 0));   // default Static: no pumping possible

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "denoise_reduction", 1 }, "Reduction",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "denoise_sensitivity", 1 }, "Sensitivity",
        juce::NormalisableRange<float> (0.5f, 3.0f, 0.01f), 1.0f,
        juce::AudioParameterFloatAttributes().withLabel ("x")));
}

void NoiseReductionModule::attachToState (juce::AudioProcessorValueTreeState& apvts)
{
    enabledParam      = apvts.getRawParameterValue (moduleId + "_enabled");
    reductionParam    = apvts.getRawParameterValue ("denoise_reduction");
    sensitivityParam  = apvts.getRawParameterValue ("denoise_sensitivity");
    modeParam         = apvts.getRawParameterValue ("denoise_mode");
}

void NoiseReductionModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    for (auto& ch : channels)
        ch.prepare();
}

void NoiseReductionModule::reset()
{
    for (auto& ch : channels)
        ch.reset();
}

void NoiseReductionModule::process (juce::AudioBuffer<float>& buffer)
{
    if (learnRequested.exchange (false, std::memory_order_relaxed))
    {
        for (auto& ch : channels)
            ch.startLearning (sampleRate, kLearnSeconds);
        learningActive.store (true, std::memory_order_relaxed);
        profileLearned.store (false, std::memory_order_relaxed);
    }

    float reduction   = reductionParam   ? juce::jlimit (0.0f, 1.0f, reductionParam->load() / 100.0f) : 0.5f;
    float sensitivity = sensitivityParam ? sensitivityParam->load() : 1.0f;
    int   mode        = modeParam ? juce::jlimit (0, 1, (int) std::round (modeParam->load())) : 0;

    int numChannels = juce::jmin (2, buffer.getNumChannels());
    int numSamples  = buffer.getNumSamples();

    bool anyFinished = false;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            bool justFinished = false;
            data[i] = channels[(size_t) ch].processSample (data[i], reduction, sensitivity, mode, justFinished);
            if (justFinished) anyFinished = true;
        }
    }

    if (anyFinished)
    {
        learningActive.store (false, std::memory_order_relaxed);
        profileLearned.store (true, std::memory_order_relaxed);
    }
}
