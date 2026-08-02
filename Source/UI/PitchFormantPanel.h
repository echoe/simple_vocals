#pragma once
#include "HalfModulePanel.h"
#include "../Modules/PitchFormantModule.h"

/** Control panel for the Pitch/Formant module — a standard module panel
    like the others, single row: Pitch, Formant, Mix knobs. This is a fixed
    transpose, NOT pitch correction — see PitchFormantModule's docs.
    Enable/disable is via the chain strip, same as every other module. */
class PitchFormantPanel : public HalfModulePanel
{
public:
    PitchFormantPanel (juce::AudioProcessorValueTreeState& apvts);

    void resized() override;

private:
    juce::Slider pitchSlider, formantSlider, mixSlider;
    juce::Label  pitchLabel,  formantLabel,  mixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchAtt, formantAtt, mixAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchFormantPanel)
};
