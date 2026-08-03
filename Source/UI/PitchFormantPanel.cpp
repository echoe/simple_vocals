#include "PitchFormantPanel.h"

PitchFormantPanel::PitchFormantPanel (juce::AudioProcessorValueTreeState& apvts)
    : HalfModulePanel ("PITCH / FORMANT", juce::Colour (0xff80c0d0))
{
    makeKnob (pitchSlider);    makeLabel (pitchLabel,    "Pitch");
    makeKnob (formantSlider);  makeLabel (formantLabel,  "Formant");
    makeKnob (mixSlider);      makeLabel (mixLabel,      "Mix");

    for (auto* s : { &pitchSlider, &formantSlider, &mixSlider })
        addAndMakeVisible (s);
    for (auto* l : { &pitchLabel, &formantLabel, &mixLabel })
        addAndMakeVisible (l);

    pitchAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "pitchfx_semitones", pitchSlider);
    formantAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "pitchfx_formant", formantSlider);
    mixAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "pitchfx_mix", mixSlider);
}

void PitchFormantPanel::resized()
{
    auto row = contentArea();   // use the full height, not a fixed 38px slice

    layoutKnobRow (row, { { &pitchLabel,   &pitchSlider   },
                           { &formantLabel, &formantSlider },
                           { &mixLabel,     &mixSlider     } });
}
