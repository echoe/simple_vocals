#pragma once
#include "ModulePanel.h"
#include "../Modules/CompressorModule.h"

/** Control panel for the Compressor module.
    Visual: live GR meter bar + a static transfer-curve sketch that updates
    as threshold/ratio/knee change.
    Controls: Threshold, Ratio, Attack, Release, Knee, Makeup — 2 rows of 3. */
class CompressorPanel : public ModulePanel
{
public:
    CompressorPanel (juce::AudioProcessorValueTreeState& apvts, CompressorModule* module);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    CompressorModule* compressor;  // may be nullptr; always null-check

    juce::Slider thresholdSlider, ratioSlider, attackSlider,
                 releaseSlider,  kneeSlider,   makeupSlider;
    juce::Label  thresholdLabel,  ratioLabel,   attackLabel,
                 releaseLabel,    kneeLabel,    makeupLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        thresholdAtt, ratioAtt, attackAtt, releaseAtt, kneeAtt, makeupAtt;

    void drawTransferCurve (juce::Graphics&, juce::Rectangle<int> area) const;
    void drawGrMeter       (juce::Graphics&, juce::Rectangle<int> area) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorPanel)
};
