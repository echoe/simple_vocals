#pragma once
#include "ModulePanel.h"
#include "../Modules/DeEsserModule.h"

/** Control panel for the De-Esser module.
    Visual: a frequency-domain diagram showing the crossover point and the
    sibilant detection zone, plus a live GR meter.
    Controls: Freq, Threshold, Range knobs. */
class DeEsserPanel : public ModulePanel
{
public:
    DeEsserPanel (juce::AudioProcessorValueTreeState& apvts, DeEsserModule* module);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    DeEsserModule* deEsser;   // may be nullptr; always null-check

    juce::Slider freqSlider, thresholdSlider, rangeSlider;
    juce::Label  freqLabel,  thresholdLabel,  rangeLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAtt, thresholdAtt, rangeAtt;

    void drawFreqViz   (juce::Graphics&, juce::Rectangle<int> area) const;
    void drawGrMeter   (juce::Graphics&, juce::Rectangle<int> area) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeEsserPanel)
};
