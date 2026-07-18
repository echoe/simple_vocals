#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/ChainStrip.h"
#include "UI/EQCurveComponent.h"
#include "UI/EQControlsStrip.h"
#include "UI/PresetBar.h"
#include "UI/AutotuneComponent.h"
#include "UI/DeEsserPanel.h"
#include "UI/CompressorPanel.h"
#include "UI/SaturationPanel.h"
#include "UI/HarmonizerPanel.h"
#include "UI/ReverbPanel.h"
#include "UI/DelayPanel.h"

class SimpleVocalsAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SimpleVocalsAudioProcessorEditor (SimpleVocalsAudioProcessor&);
    ~SimpleVocalsAudioProcessorEditor() override = default;

    void paint   (juce::Graphics&) override;
    void resized () override;

    static constexpr int kMargin      = 8;
    static constexpr int kPresetBarH  = 30;
    static constexpr int kChainH      = 56;   // horizontal chain strip, top of window
    static constexpr int kEQH         = 250;
    static constexpr int kEQControlsH = 80;
    static constexpr int kAutoH       = 110;
    static constexpr int kLeftColW    = 560;  // EQ + Autotune column (narrowed to make room for the module grid)

private:
    SimpleVocalsAudioProcessor& audioProcessor;

    PresetBar         presetBar;
    ChainStrip        chainStrip;
    EQCurveComponent  eqCurve;
    EQControlsStrip   eqControls;
    AutotuneComponent autotuneStrip;

    DeEsserPanel    deEsserPanel;
    CompressorPanel compressorPanel;
    DelayPanel      delayPanel;
    SaturationPanel saturationPanel;
    HarmonizerPanel harmonizerPanel;
    ReverbPanel     reverbPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleVocalsAudioProcessorEditor)
};
