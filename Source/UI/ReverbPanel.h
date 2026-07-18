#pragma once
#include "ModulePanel.h"

/** Control panel for the Reverb module.
    Visual: a decay-tail diagram showing the pre-delay gap followed by an
    exponential envelope. Size and damping change the slope; pre-delay shifts
    the tail onset; the mix level dims the whole envelope.
    Controls: Size, Damping, Width (row 1) · Pre-delay, Mix, Freeze (row 2) */
class ReverbPanel : public ModulePanel
{
public:
    ReverbPanel (juce::AudioProcessorValueTreeState& apvts);

    void resized() override;
    void paint   (juce::Graphics&) override;

private:
    juce::Slider sizeSlider, dampingSlider, widthSlider,
                 predelaySlider, mixSlider;
    juce::Label  sizeLabel,  dampingLabel,  widthLabel,
                 predelayLabel, mixLabel;

    juce::ToggleButton freezeButton { "Freeze" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        sizeAtt, dampingAtt, widthAtt, predelayAtt, mixAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        freezeAtt;

    void drawDecayTail (juce::Graphics&, juce::Rectangle<int> area) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbPanel)
};
