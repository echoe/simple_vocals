#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Modules/EQModule.h"

/** Horizontal strip of compact controls for all 8 EQ bands.

    Each column shows:
      • A small enable toggle + band number label in the header row
      • Three LinearBar sliders: Q · Threshold · Ratio

    Q/Threshold/Ratio are already in the APVTS (registered by EQModule);
    this strip just provides visible, draggable controls for parameters that
    were previously only reachable via scroll wheel (Q) or not reachable at
    all (Threshold, Ratio) unless the band is in dynamic mode.

    Having Threshold and Ratio always visible even on non-dynamic bands lets
    you pre-configure them before double-clicking a node to engage dynamic
    mode — useful when dialling in an OTT-style multi-band dynamic effect. */
class EQControlsStrip : public juce::Component
{
public:
    explicit EQControlsStrip (juce::AudioProcessorValueTreeState& apvts);

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    struct BandRow
    {
        juce::ToggleButton enableToggle;
        juce::Slider       qSlider, threshSlider, ratioSlider;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAtt, threshAtt, ratioAtt;
    };

    std::array<BandRow, EQModule::maxBands> bands;

    static void setupSlider (juce::Slider& s);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQControlsStrip)
};
