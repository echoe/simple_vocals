#pragma once
#include "HalfModulePanel.h"
#include "../Modules/NoiseReductionModule.h"

/** Control panel for the Noise Reduction ("Denoise") module — a standard
    module panel like the others, single row of controls: Learn Noise
    Profile button, Static/Adaptive mode pair, Reduction and Sensitivity
    knobs. Enable/disable is via the chain strip, same as every other
    module (no separate on/off switch here, for consistency).
    Workflow: click Learn while only room tone/hiss is playing (no vocal),
    wait ~1.5s, then dial in Reduction and Sensitivity. */
class DenoisePanel : public HalfModulePanel
{
public:
    DenoisePanel (juce::AudioProcessorValueTreeState& apvts, NoiseReductionModule* module);

    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    NoiseReductionModule* denoiseModule;   // may be nullptr; always null-check

    juce::TextButton learnButton { "Learn Noise" };
    juce::TextButton staticModeButton   { "Static" };
    juce::TextButton adaptiveModeButton { "Adaptive" };
    void updateModeButtons();

    juce::Slider reductionSlider, sensitivitySlider;
    juce::Label  reductionLabel,  sensitivityLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reductionAtt, sensitivityAtt;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DenoisePanel)
};
