#include "PluginProcessor.h"
#include "PluginEditor.h"

SimpleVocalsAudioProcessor::SimpleVocalsAudioProcessor()
    : AudioProcessor (BusesProperties()
                           .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      effectChain(),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout()),
      presetManager (apvts, effectChain)
{
    effectChain.attachToState (apvts);
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleVocalsAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    effectChain.addParameters (layout);
    return layout;
}

void SimpleVocalsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();

    effectChain.prepare (spec);

    hfAnalysisFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 5000.0f);
    hfAnalysisFilter.reset();

    setLatencySamples (effectChain.getTotalLatencySamples());
}

void SimpleVocalsAudioProcessor::startChainAnalysis() noexcept
{
    analysisRmsDb.store   (-60.0f, std::memory_order_relaxed);
    analysisPeakDb.store  (-60.0f, std::memory_order_relaxed);
    analysisHfRatio.store (0.0f,   std::memory_order_relaxed);
    hfAnalysisFilter.reset();
    chainAnalysisActive.store (true, std::memory_order_relaxed);
}

void SimpleVocalsAudioProcessor::updateChainAnalysis (const juce::AudioBuffer<float>& dryBuffer)
{
    int n = dryBuffer.getNumSamples();
    int chans = dryBuffer.getNumChannels();
    if (n <= 0 || chans <= 0) return;

    float sumSq = 0.0f, peak = 0.0f, hfSumSq = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        float mono = 0.0f;
        for (int c = 0; c < chans; ++c) mono += dryBuffer.getSample (c, i);
        mono /= (float) chans;

        sumSq += mono * mono;
        peak   = juce::jmax (peak, std::abs (mono));

        float hf = hfAnalysisFilter.processSample (mono);
        hfSumSq += hf * hf;
    }

    float blockRms   = std::sqrt (sumSq   / (float) n);
    float blockHfRms = std::sqrt (hfSumSq / (float) n);
    float blockRmsDb  = juce::Decibels::gainToDecibels (blockRms, -100.0f);
    float blockPeakDb = juce::Decibels::gainToDecibels (peak,     -100.0f);
    float hfRatio      = (blockRms > 1.0e-6f) ? juce::jlimit (0.0f, 1.0f, blockHfRms / blockRms) : 0.0f;

    // One-pole smoothing per block so short transients don't dominate the read.
    constexpr float smooth = 0.15f;
    auto smoothStore = [&] (std::atomic<float>& a, float target)
    {
        float cur = a.load (std::memory_order_relaxed);
        a.store (cur + (target - cur) * smooth, std::memory_order_relaxed);
    };
    smoothStore (analysisRmsDb,   blockRmsDb);
    smoothStore (analysisPeakDb,  blockPeakDb);
    smoothStore (analysisHfRatio, hfRatio);
}

void SimpleVocalsAudioProcessor::releaseResources()
{
}

bool SimpleVocalsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void SimpleVocalsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    if (chainAnalysisActive.load (std::memory_order_relaxed))
        updateChainAnalysis (buffer);   // tap the dry signal before the chain mutates it in place

    effectChain.process (buffer);

    // Latency (e.g. Autotune's Live/Studio grain size, or enabling/disabling
    // Autotune) can change between blocks. Only call setLatencySamples() when
    // it actually changed — it's a comparatively heavy host notification.
    int newLatency = effectChain.getTotalLatencySamples();
    if (newLatency != getLatencySamples())
        setLatencySamples (newLatency);
}

juce::AudioProcessorEditor* SimpleVocalsAudioProcessor::createEditor()
{
    return new SimpleVocalsAudioProcessorEditor (*this);
}

void SimpleVocalsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    effectChain.appendOrderToState (state);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SimpleVocalsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid())
        {
            effectChain.restoreOrderFromState (state);
            apvts.replaceState (state);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleVocalsAudioProcessor();
}
