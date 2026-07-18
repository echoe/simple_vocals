#include "DeEsserModule.h"

void DeEsserModule::addParameters (juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    EffectModule::addParameters (layout);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "deess_freq", 1 }, "Freq",
        juce::NormalisableRange<float> (2000.0f, 16000.0f, 1.0f, 0.35f), 6000.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "deess_threshold", 1 }, "Threshold",
        juce::NormalisableRange<float> (-40.0f, 0.0f, 0.1f), -18.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "deess_range", 1 }, "Range (dB)",
        juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 12.0f));
}

void DeEsserModule::attachToState (juce::AudioProcessorValueTreeState& apvts)
{
    EffectModule::attachToState (apvts);
    freqParam      = apvts.getRawParameterValue ("deess_freq");
    thresholdParam = apvts.getRawParameterValue ("deess_threshold");
    rangeParam     = apvts.getRawParameterValue ("deess_range");
}

void DeEsserModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    juce::dsp::ProcessSpec mono { spec.sampleRate, spec.maximumBlockSize, 1 };

    auto lpCoefs = FilterCoefs::makeLowPass  (sampleRate, 6000.0, 0.707);
    auto hpCoefs = FilterCoefs::makeHighPass (sampleRate, 6000.0, 0.707);

    lpL.coefficients = lpCoefs;  lpR.coefficients = lpCoefs;
    hpL.coefficients = hpCoefs;  hpR.coefficients = hpCoefs;

    lpL.prepare (mono); lpR.prepare (mono);
    hpL.prepare (mono); hpR.prepare (mono);

    envelopeDb = -100.0f;
    grMeter.store (0.0f, std::memory_order_relaxed);
}

void DeEsserModule::process (juce::AudioBuffer<float>& buffer)
{
    auto freq        = freqParam      != nullptr ? freqParam->load()      : 6000.0f;
    auto thresholdDb = thresholdParam != nullptr ? thresholdParam->load() : -18.0f;
    auto rangeDb     = rangeParam     != nullptr ? rangeParam->load()     : 12.0f;
    auto numSamples  = buffer.getNumSamples();
    auto numCh       = buffer.getNumChannels();

    auto lpCoefs = FilterCoefs::makeLowPass  (sampleRate, (double) freq, 0.707);
    auto hpCoefs = FilterCoefs::makeHighPass (sampleRate, (double) freq, 0.707);
    lpL.coefficients = lpCoefs;  lpR.coefficients = lpCoefs;
    hpL.coefficients = hpCoefs;  hpR.coefficients = hpCoefs;

    // Pre-compute per-sample attack/release coefficients
    auto attackCoeff  = 1.0f - std::exp (-1.0f / (float) (sampleRate * 0.002));  // 2ms
    auto releaseCoeff = 1.0f - std::exp (-1.0f / (float) (sampleRate * 0.080));  // 80ms

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = numCh >= 1 ? buffer.getSample (0, s) : 0.0f;
        float inR = numCh >= 2 ? buffer.getSample (1, s) : inL;

        // Split into low and high bands
        float lowL = lpL.processSample (inL);
        float lowR = lpR.processSample (inR);
        float highL = hpL.processSample (inL);
        float highR = hpR.processSample (inR);

        // Detect peak in the high band (both channels linked)
        float peak     = std::max (std::abs (highL), std::abs (highR));
        float peakDb   = juce::Decibels::gainToDecibels (peak, -100.0f);
        auto  coeff    = peakDb > envelopeDb ? attackCoeff : releaseCoeff;
        envelopeDb    += (peakDb - envelopeDb) * coeff;

        // Compute gain reduction
        float grDb   = 0.0f;
        float excess = envelopeDb - thresholdDb;
        if (excess > 0.0f)
            grDb = -std::min (excess, rangeDb);

        float grLinear = juce::Decibels::decibelsToGain (grDb);

        // Recombine: low passes through, high is attenuated
        if (numCh >= 1) buffer.setSample (0, s, lowL + highL * grLinear);
        if (numCh >= 2) buffer.setSample (1, s, lowR + highR * grLinear);
    }

    grMeter.store (envelopeDb > thresholdDb
                       ? -std::min (envelopeDb - thresholdDb, rangeDb)
                       : 0.0f,
                   std::memory_order_relaxed);
}

void DeEsserModule::reset()
{
    lpL.reset(); lpR.reset();
    hpL.reset(); hpR.reset();
    envelopeDb = -100.0f;
    grMeter.store (0.0f, std::memory_order_relaxed);
}
