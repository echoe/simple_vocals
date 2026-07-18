#pragma once
#include "ModulePanel.h"
#include "../Modules/SaturationModule.h"

/** Control panel for the Saturation module.
    Visual: live transfer-curve that redraws as Drive and Mode change,
    so users immediately see the waveshaping character of each mode.
    Controls: Mode (ComboBox), Drive, Tone, Mix, Output. */
class SaturationPanel : public ModulePanel
{
public:
    SaturationPanel (juce::AudioProcessorValueTreeState& apvts);

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    juce::AudioProcessorValueTreeState& apvts;   // kept for timer sync

    juce::ComboBox modeBox;
    juce::Label    modeLabel;

    juce::Slider driveSlider, toneSlider, mixSlider, outputSlider;
    juce::Label  driveLabel,  toneLabel,  mixLabel,  outputLabel;

    // No ComboBoxAttachment — manual sync used instead (version-independent)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   driveAtt,
                                                                             toneAtt,
                                                                             mixAtt,
                                                                             outputAtt;

    /** Keeps the combo in sync with the parameter during host automation. */
    void timerCallback() override;

    void drawTransferCurve (juce::Graphics&, juce::Rectangle<int> area) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SaturationPanel)
};
