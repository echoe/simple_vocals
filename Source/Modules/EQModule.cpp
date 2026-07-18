#include "EQModule.h"

namespace
{
    // Default frequencies spread across the vocal range
    constexpr float defaultFreqs[EQModule::maxBands] = {
        80.0f, 250.0f, 800.0f, 3000.0f, 6000.0f, 10000.0f, 14000.0f, 18000.0f
    };
    constexpr float defaultQs[EQModule::maxBands] = {
        0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 0.8f, 0.7f, 0.7f
    };
    constexpr bool defaultEnabled[EQModule::maxBands] = {
        true, true, true, true, false, false, false, false
    };
}

void EQModule::addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    EffectModule::addParameters (layout);

    juce::NormalisableRange<float> freqRange      (20.0f,  20000.0f, 1.0f,  0.3f);
    juce::NormalisableRange<float> gainRange      (-18.0f, 18.0f,    0.1f);
    juce::NormalisableRange<float> qRange         (0.2f,   8.0f,     0.01f, 0.4f);
    juce::NormalisableRange<float> thresholdRange (-60.0f, 0.0f,     0.1f);
    juce::NormalisableRange<float> ratioRange     (1.0f,   20.0f,    0.1f,  0.4f);

    for (int i = 0; i < maxBands; ++i)
    {
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { bandParamID (i, "enabled"), 1 },
            "Band " + juce::String (i + 1) + " Enabled", defaultEnabled[i]));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { bandParamID (i, "freq"), 1 },
            "Band " + juce::String (i + 1) + " Freq", freqRange, defaultFreqs[i]));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { bandParamID (i, "gain"), 1 },
            "Band " + juce::String (i + 1) + " Gain", gainRange, 0.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { bandParamID (i, "q"), 1 },
            "Band " + juce::String (i + 1) + " Q", qRange, defaultQs[i]));
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { bandParamID (i, "dynamic"), 1 },
            "Band " + juce::String (i + 1) + " Dynamic", false));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { bandParamID (i, "threshold"), 1 },
            "Band " + juce::String (i + 1) + " Threshold", thresholdRange, -20.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { bandParamID (i, "ratio"), 1 },
            "Band " + juce::String (i + 1) + " Ratio", ratioRange, 4.0f));
    }
}

void EQModule::attachToState (juce::AudioProcessorValueTreeState& apvts)
{
    EffectModule::attachToState (apvts);

    for (int i = 0; i < maxBands; ++i)
    {
        auto& band = bands[(size_t) i];
        band.enabledParam    = apvts.getRawParameterValue (bandParamID (i, "enabled"));
        band.freqParam       = apvts.getRawParameterValue (bandParamID (i, "freq"));
        band.gainParam       = apvts.getRawParameterValue (bandParamID (i, "gain"));
        band.qParam          = apvts.getRawParameterValue (bandParamID (i, "q"));
        band.dynamicParam    = apvts.getRawParameterValue (bandParamID (i, "dynamic"));
        band.thresholdParam  = apvts.getRawParameterValue (bandParamID (i, "threshold"));
        band.ratioParam      = apvts.getRawParameterValue (bandParamID (i, "ratio"));
    }
}

void EQModule::updateBandCoefficients (Band& band, float freq, float q, float gainLinear)
{
    auto coefs = FilterCoefs::makePeakFilter (sampleRate, freq, q, gainLinear);
    band.filterL.coefficients = coefs;
    band.filterR.coefficients = coefs;
}

void EQModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    juce::dsp::ProcessSpec monoSpec = { spec.sampleRate, spec.maximumBlockSize, 1 };

    for (int i = 0; i < maxBands; ++i)
    {
        auto& band = bands[(size_t) i];
        auto  freq  = defaultFreqs[i];

        // Assign valid coefficients BEFORE calling prepare(), otherwise the
        // filter's internal state is uninitialised.
        auto processCoefs = FilterCoefs::makePeakFilter (sampleRate, freq, defaultQs[i], 1.0f);
        auto detectCoefs  = FilterCoefs::makeBandPass  (sampleRate, freq, defaultQs[i]);
        band.filterL.coefficients = processCoefs;
        band.filterR.coefficients = processCoefs;
        band.detectL.coefficients = detectCoefs;
        band.detectR.coefficients = detectCoefs;

        band.filterL.prepare (monoSpec);
        band.filterR.prepare (monoSpec);
        band.detectL.prepare (monoSpec);
        band.detectR.prepare (monoSpec);

        band.envelopeDb = -100.0f;
    }
}

float EQModule::computeTargetGainDb (Band& band, const juce::AudioBuffer<float>& buffer)
{
    auto baseGainDb = band.gainParam  != nullptr ? band.gainParam->load()  : 0.0f;
    auto isDynamic  = band.dynamicParam != nullptr && band.dynamicParam->load() > 0.5f;

    if (! isDynamic)
        return baseGainDb;

    auto freq        = band.freqParam       != nullptr ? band.freqParam->load()       : 1000.0f;
    auto q           = band.qParam          != nullptr ? band.qParam->load()          : 1.0f;
    auto thresholdDb = band.thresholdParam  != nullptr ? band.thresholdParam->load()  : -20.0f;
    auto ratio       = band.ratioParam      != nullptr ? band.ratioParam->load()      : 4.0f;

    auto detectCoefs = FilterCoefs::makeBandPass (sampleRate, freq, q);
    band.detectL.coefficients = detectCoefs;
    band.detectR.coefficients = detectCoefs;

    // Measure peak level in this frequency band via a scratch pass of the detect filters.
    // We work on copies of the samples so the detect filters don't consume the signal.
    float peak = 0.0f;
    auto numSamples = buffer.getNumSamples();

    if (buffer.getNumChannels() >= 1)
    {
        const auto* src = buffer.getReadPointer (0);
        for (int s = 0; s < numSamples; ++s)
            peak = juce::jmax (peak, std::abs (band.detectL.processSample (src[s])));
    }
    if (buffer.getNumChannels() >= 2)
    {
        const auto* src = buffer.getReadPointer (1);
        for (int s = 0; s < numSamples; ++s)
            peak = juce::jmax (peak, std::abs (band.detectR.processSample (src[s])));
    }

    auto blockPeakDb     = juce::Decibels::gainToDecibels (peak, -100.0f);
    auto blockSecs       = sampleRate > 0.0 ? (double) numSamples / sampleRate : 0.01;
    auto timeConstantMs  = blockPeakDb > band.envelopeDb ? 10.0 : 150.0;
    auto coeff           = 1.0f - std::exp ((float) (-blockSecs / (timeConstantMs / 1000.0)));
    band.envelopeDb     += (blockPeakDb - band.envelopeDb) * coeff;

    auto excess = band.envelopeDb - thresholdDb;
    if (excess <= 0.0f)
        return baseGainDb;

    auto reduction = excess * (1.0f - 1.0f / ratio);
    return baseGainDb >= 0.0f ? juce::jmax (0.0f, baseGainDb - reduction)
                              : juce::jmin (0.0f, baseGainDb + reduction);
}

void EQModule::process (juce::AudioBuffer<float>& buffer)
{
    for (int i = 0; i < maxBands; ++i)
    {
        auto& band = bands[(size_t) i];

        if (band.enabledParam != nullptr && band.enabledParam->load() < 0.5f)
            continue; // band is disabled

        auto freq       = band.freqParam  != nullptr ? band.freqParam->load()  : 1000.0f;
        auto q          = band.qParam     != nullptr ? band.qParam->load()     : 1.0f;
        auto targetGain = computeTargetGainDb (band, buffer);
        auto gainLinear = juce::Decibels::decibelsToGain (targetGain);

        // Assign fresh coefficients directly to each filter's coefficients pointer.
        // This is safe because filterL/filterR are independent objects; no shared pointer.
        auto coefs = FilterCoefs::makePeakFilter (sampleRate, freq, q, gainLinear);
        band.filterL.coefficients = coefs;
        band.filterR.coefficients = coefs;

        auto numSamples = buffer.getNumSamples();

        if (buffer.getNumChannels() >= 1)
        {
            auto* ch = buffer.getWritePointer (0);
            for (int s = 0; s < numSamples; ++s)
                ch[s] = band.filterL.processSample (ch[s]);
        }
        if (buffer.getNumChannels() >= 2)
        {
            auto* ch = buffer.getWritePointer (1);
            for (int s = 0; s < numSamples; ++s)
                ch[s] = band.filterR.processSample (ch[s]);
        }
    }
}

void EQModule::reset()
{
    for (auto& band : bands)
    {
        band.filterL.reset();
        band.filterR.reset();
        band.detectL.reset();
        band.detectR.reset();
        band.envelopeDb = -100.0f;
    }
}
