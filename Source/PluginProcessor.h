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

    // ── Chain analysis (for the "Auto Chain" feature) ───────────────────────
    // While active, taps the dry input signal (before the effect chain
    // processes it) and maintains smoothed level/tone statistics that the UI
    // can read to build a heuristic starting chain configuration.
    void startChainAnalysis() noexcept;
    void stopChainAnalysis()  noexcept { chainAnalysisActive.store (false, std::memory_order_relaxed); }
    bool isChainAnalysisActive() const noexcept { return chainAnalysisActive.load (std::memory_order_relaxed); }

    float getAnalysisRmsDb()   const noexcept { return analysisRmsDb.load   (std::memory_order_relaxed); }
    float getAnalysisPeakDb()  const noexcept { return analysisPeakDb.load  (std::memory_order_relaxed); }
    float getAnalysisHfRatio() const noexcept { return analysisHfRatio.load (std::memory_order_relaxed); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateChainAnalysis (const juce::AudioBuffer<float>& dryBuffer);

    std::atomic<bool>  chainAnalysisActive { false };
    std::atomic<float> analysisRmsDb   { -60.0f };  // smoothed overall level
    std::atomic<float> analysisPeakDb  { -60.0f };  // smoothed peak level (peak - rms ≈ crest factor)
    std::atomic<float> analysisHfRatio { 0.0f };    // smoothed high-band / total energy, a sibilance proxy

    juce::dsp::IIR::Filter<float> hfAnalysisFilter; // ~5 kHz highpass, mono, analysis-only

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleVocalsAudioProcessor)
};
