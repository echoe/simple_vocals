#pragma once
#include "ModulePanel.h"

class DelayPanel : public ModulePanel
{
public:
    DelayPanel (juce::AudioProcessorValueTreeState& apvts);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    juce::Slider timeSlider, feedbackSlider, mixSlider, toneSlider;
    juce::Label  timeLabel,  feedbackLabel,  mixLabel,  toneLabel;
    juce::ToggleButton pingPongButton { "Ping-Pong" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        timeAtt, feedbackAtt, mixAtt, toneAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pingPongAtt;

    void drawEchoViz (juce::Graphics&, juce::Rectangle<int> area) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayPanel)
};
