#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Modules/EQModule.h"

/** Horizontal strip of controls for all 4 EQ bands.

    Each column shows:
      • A header with the band number + an enable toggle
      • Three labeled sliders (Q · Threshold · Ratio), each with a visible
        numeric readout — these were previously bare, unlabeled bars with
        no way to read the current value

    Q/Threshold/Ratio are already in the APVTS (registered by EQModule);
    this strip just provides visible, draggable, readable controls for
    parameters that were previously only reachable via scroll wheel (Q) or
    not reachable at all (Threshold, Ratio) unless the band is in dynamic
    mode.

    Having Threshold and Ratio always visible even on non-dynamic bands lets
    you pre-configure them before double-clicking a node to engage dynamic
    mode — useful when dialling in an OTT-style multi-band dynamic effect.

    Band count was trimmed from 8 to 4 so each column has enough width for
    a readable label + slider + number — none of the factory presets used
    bands 5-8 anyway. */
class EQControlsStrip : public juce::Component
{
public:
    explicit EQControlsStrip (juce::AudioProcessorValueTreeState& apvts,
                               juce::String idPrefix = "eq");

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    struct BandRow
    {
        juce::ToggleButton enableToggle;
        juce::Slider       qSlider, threshSlider, ratioSlider;
        juce::Label        qLabel, threshLabel, ratioLabel;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAtt, threshAtt, ratioAtt;
    };

    std::array<BandRow, EQModule::maxBands> bands;

    static void setupSlider (juce::Slider& s, juce::Colour thumbColour);
    static void setupLabel  (juce::Label& l, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQControlsStrip)
};
