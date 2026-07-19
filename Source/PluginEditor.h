#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/ChainStrip.h"
#include "UI/AutoChainButton.h"
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
    static constexpr int kEQTabH      = 20;   // EQ 1 / EQ 2 tab selector
    static constexpr int kEQH         = 250;
    static constexpr int kEQControlsH = 80;
    static constexpr int kAutoH       = 110;
    static constexpr int kLeftColW    = 560;  // EQ + Autotune column (narrowed to make room for the module grid)

private:
    SimpleVocalsAudioProcessor& audioProcessor;

    PresetBar         presetBar;
    AutoChainButton   autoChainButton;
    ChainStrip        chainStrip;

    // EQ 1 and EQ 2 are two independent EQModule instances (see EffectChain).
    // Both share the same on-screen real estate; a small tab selector picks
    // which one's curve + band controls are shown/edited. Each still has its
    // own slot in the horizontal chain strip up top, so they can be placed
    // at different points in the signal flow regardless of which is being
    // viewed here.
    juce::TextButton  eq1TabButton { "EQ 1" };
    juce::TextButton  eq2TabButton { "EQ 2" };
    int               selectedEqTab = 0;   // 0 = EQ 1, 1 = EQ 2
    EQCurveComponent  eqCurve1;
    EQCurveComponent  eqCurve2;
    EQControlsStrip   eqControls1;
    EQControlsStrip   eqControls2;

    AutotuneComponent autotuneStrip;

    DeEsserPanel    deEsserPanel;
    CompressorPanel compressorPanel;
    DelayPanel      delayPanel;
    SaturationPanel saturationPanel;
    HarmonizerPanel harmonizerPanel;
    ReverbPanel     reverbPanel;

    void selectEqTab (int tab);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleVocalsAudioProcessorEditor)
};
