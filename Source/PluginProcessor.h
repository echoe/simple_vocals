#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "EffectChain.h"
#include "PresetManager.h"

class SimpleVocalsAudioProcessor : public juce::AudioProcessor
{
public:
    SimpleVocalsAudioProcessor();
    ~SimpleVocalsAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    // Bring the double-precision overload into scope so it isn't hidden.
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    EffectChain effectChain;                       // must be declared BEFORE apvts:
    juce::AudioProcessorValueTreeState apvts;       // createParameterLayout() needs effectChain alive
    PresetManager presetManager;                    // must be after both effectChain and apvts

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleVocalsAudioProcessor)
};
