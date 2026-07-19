#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"

/** Single-button "Auto Chain" tool.

    Click it and it listens to ~4 seconds of dry input, measures level,
    dynamics (crest factor), and high-frequency/sibilance energy, then
    applies a heuristic starting configuration across the whole module
    chain: a standard vocal low-cut + light presence on EQ 1, a compressor
    scaled to the measured dynamics, a de-esser scaled to measured
    sibilance, light saturation and reverb as "glue", and a neutral
    chromatic Autotune (pairs with the separate Auto Key button if a
    specific key is wanted). Harmonizer, Delay, and EQ 2 are left off since
    those are stylistic choices rather than corrective ones. Finally resets
    the chain to its default processing order.

    This is rule-based signal analysis, not a trained model — it's meant as
    a fast, sensible starting point to tweak from, not a finished mix. */
class AutoChainButton : public juce::Component,
                        private juce::Timer
{
public:
    AutoChainButton (SimpleVocalsAudioProcessor& proc,
                     juce::AudioProcessorValueTreeState& apvts,
                     EffectChain& chain);
    ~AutoChainButton() override;

    void resized() override;

private:
    SimpleVocalsAudioProcessor&          processor;
    juce::AudioProcessorValueTreeState&  apvts;
    EffectChain&                         effectChain;

    juce::TextButton button { "Auto Chain" };

    bool active = false;
    int  ticks  = 0;
    static constexpr int kMaxTicks = 120;   // ~4s at 30Hz

    void start();
    void finish();
    void timerCallback() override;
    void showResultThenRevert (const juce::String& text);

    void setParam (const juce::String& id, float value);
    void setBool  (const juce::String& id, bool value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoChainButton)
};
