#pragma once
#include "ModulePanel.h"

/** Control panel for the Harmonizer module.
    Visual: a semitone axis (-12 to +12) with a "dry" marker at 0 and
    two coloured markers for the two harmony voices. Interval names
    (M3, P5, Oct…) are shown inside each marker.
    Controls: Interval 1, Interval 2, Mix, Grain. */
class HarmonizerPanel : public ModulePanel
{
public:
    HarmonizerPanel (juce::AudioProcessorValueTreeState& apvts);

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    juce::Slider int1Slider, int2Slider, mixSlider, grainSlider,
                 humaniseSlider, widthSlider;
    juce::Label  int1Label,  int2Label,  mixLabel,  grainLabel,
                 humaniseLabel, widthLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        int1Att, int2Att, mixAtt, grainAtt, humaniseAtt, widthAtt;

    void drawIntervalMap (juce::Graphics&, juce::Rectangle<int> area) const;

    static juce::String intervalName (int semitones);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonizerPanel)
};
