#include "CompressorModule.h"

void CompressorModule::addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    EffectModule::addParameters (layout);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "comp_threshold", 1 }, "Threshold",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -18.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "comp_ratio", 1 }, "Ratio",
        juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f, 0.4f), 3.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "comp_attack", 1 }, "Attack (ms)",
        juce::NormalisableRange<float> (0.1f, 100.0f, 0.1f, 0.4f), 10.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "comp_release", 1 }, "Release (ms)",
        juce::NormalisableRange<float> (10.0f, 1000.0f, 1.0f, 0.4f), 100.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "comp_knee", 1 }, "Knee (dB)",
        juce::NormalisableRange<float> (0.0f, 12.0f, 0.1f), 2.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "comp_makeup", 1 }, "Makeup (dB)",
        juce::NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f));
}

void CompressorModule::attachToState (juce::AudioProcessorValueTreeState& apvts)
{
    EffectModule::attachToState (apvts);

    thresholdParam = apvts.getRawParameterValue ("comp_threshold");
    ratioParam     = apvts.getRawParameterValue ("comp_ratio");
    attackParam    = apvts.getRawParameterValue ("comp_attack");
    releaseParam   = apvts.getRawParameterValue ("comp_release");
    kneeParam      = apvts.getRawParameterValue ("comp_knee");
    makeupParam    = apvts.getRawParameterValue ("comp_makeup");
}

void CompressorModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate  = spec.sampleRate;
    grSmoothed  = 0.0f;
    updateCoefficients();
    grMeter.store (0.0f, std::memory_order_relaxed);
}

float CompressorModule::staticCurve (float inputDb, float threshold,
                                      float ratio, float kneeDb) const noexcept
{
    // Standard soft-knee gain computer (AES convention).
    // Returns gain reduction in dB (always <= 0).
    auto over = inputDb - threshold;

    if (kneeDb > 0.0f && 2.0f * std::abs (over) <= kneeDb)
    {
        // Inside the knee: quadratic transition
        auto t = over + kneeDb * 0.5f;
        return (1.0f / ratio - 1.0f) * t * t / (2.0f * kneeDb);
    }

    if (over <= -kneeDb * 0.5f)
        return 0.0f;                           // below knee, no compression

    return over * (1.0f / ratio - 1.0f);       // above knee, full ratio
}

void CompressorModule::updateCoefficients()
{
    auto attackMs  = attackParam  != nullptr ? attackParam->load()  : 10.0f;
    auto releaseMs = releaseParam != nullptr ? releaseParam->load() : 100.0f;

    attackCoeff  = 1.0f - std::exp (-1.0f / (float) (sampleRate * (double) attackMs  / 1000.0));
    releaseCoeff = 1.0f - std::exp (-1.0f / (float) (sampleRate * (double) releaseMs / 1000.0));
}

void CompressorModule::process (juce::AudioBuffer<float>& buffer)
{
    updateCoefficients();

    auto threshold = thresholdParam != nullptr ? thresholdParam->load() : -18.0f;
    auto ratio     = ratioParam     != nullptr ? ratioParam->load()     : 3.0f;
    auto knee      = kneeParam      != nullptr ? kneeParam->load()      : 2.0f;
    auto makeupDb  = makeupParam    != nullptr ? makeupParam->load()    : 0.0f;
    auto makeupGain = juce::Decibels::decibelsToGain (makeupDb);

    auto numSamples  = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    for (int s = 0; s < numSamples; ++s)
    {
        // Stereo-linked peak detection
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            peak = std::max (peak, std::abs (buffer.getSample (ch, s)));

        float inputDb  = juce::Decibels::gainToDecibels (peak, -100.0f);
        float targetGr = staticCurve (inputDb, threshold, ratio, knee);

        // Ballistics on the gain signal (not level): attack when compressing more,
        // release when compressing less.
        auto coeff = targetGr < grSmoothed ? attackCoeff : releaseCoeff;
        grSmoothed += (targetGr - grSmoothed) * coeff;

        float totalGain = juce::Decibels::decibelsToGain (grSmoothed) * makeupGain;
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.getWritePointer (ch)[s] *= totalGain;
    }

    grMeter.store (grSmoothed, std::memory_order_relaxed);
}

void CompressorModule::reset()
{
    grSmoothed = 0.0f;
    grMeter.store (0.0f, std::memory_order_relaxed);
}
