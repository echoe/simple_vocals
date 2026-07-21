#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Modules/NoiseReductionModule.h"

/** Slim horizontal strip for the Denoise module — sits under the 2x3 grid
    of full module panels rather than taking a grid slot itself, since it
    only needs three controls: Learn Noise Profile, Reduction, Sensitivity.
    Workflow: click Learn while only room tone/hiss is playing (no vocal),
    wait ~1.5s, then dial in Reduction and Sensitivity. */
class DenoiseBar : public juce::Component,
                   private juce::Timer
{
public:
    DenoiseBar (juce::AudioProcessorValueTreeState& apvts, NoiseReductionModule* module);
    ~DenoiseBar() override;

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    NoiseReductionModule* denoiseModule;   // may be nullptr; always null-check

    juce::TextButton learnButton { "Learn Noise Profile" };
    juce::Label      statusLabel;

    juce::Slider reductionSlider, sensitivitySlider;
    juce::Label  reductionLabel,  sensitivityLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reductionAtt, sensitivityAtt;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DenoiseBar)
};
