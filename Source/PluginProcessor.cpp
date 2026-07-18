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

    effectChain.process (buffer);
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
