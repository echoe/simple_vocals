#include "PluginEditor.h"

SimpleVocalsAudioProcessorEditor::SimpleVocalsAudioProcessorEditor (SimpleVocalsAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor  (p),
      presetBar       (p.presetManager),
      chainStrip      (p.effectChain, p.apvts),
      eqCurve         (p.apvts),
      eqControls      (p.apvts),
      autotuneStrip   (p.apvts, p.effectChain.getModuleOfType<AutotuneModule>()),
      deEsserPanel    (p.apvts, p.effectChain.getModuleOfType<DeEsserModule>()),
      compressorPanel (p.apvts, p.effectChain.getModuleOfType<CompressorModule>()),
      delayPanel      (p.apvts),
      saturationPanel (p.apvts),
      harmonizerPanel (p.apvts),
      reverbPanel     (p.apvts)
{
    addAndMakeVisible (presetBar);
    addAndMakeVisible (chainStrip);
    addAndMakeVisible (eqCurve);
    addAndMakeVisible (eqControls);
    addAndMakeVisible (autotuneStrip);
    addAndMakeVisible (deEsserPanel);
    addAndMakeVisible (compressorPanel);
    addAndMakeVisible (delayPanel);
    addAndMakeVisible (saturationPanel);
    addAndMakeVisible (harmonizerPanel);
    addAndMakeVisible (reverbPanel);

    constexpr int rightW    = 798;
    constexpr int panelRowH = 220;
    int rightH = kPresetBarH + kMargin
               + kEQH         + kMargin
               + kEQControlsH + kMargin
               + kAutoH        + kMargin
               + panelRowH    + kMargin
               + panelRowH    + kMargin;
    int chainH = kPresetBarH + kMargin
               + EffectChain::numModules * 38 + 18 + kMargin;
    setSize (kChainW + kMargin * 3 + rightW, std::max (chainH, rightH));
}

void SimpleVocalsAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff14141a));

    // Chain strip background panel
    g.setColour (juce::Colours::white.withAlpha (0.03f));
    g.fillRoundedRectangle (
        juce::Rectangle<int> (kMargin, kPresetBarH + kMargin,
                              kChainW, getHeight() - kPresetBarH - kMargin * 2).toFloat(), 4.0f);

    // Separator between chain and right area
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawLine ((float) (kMargin + kChainW + kMargin / 2),
                (float) (kPresetBarH + kMargin),
                (float) (kMargin + kChainW + kMargin / 2),
                (float) (getHeight() - kMargin), 1.0f);
}

void SimpleVocalsAudioProcessorEditor::resized()
{
    auto full = getLocalBounds();

    // Preset bar — full width at top
    presetBar.setBounds (full.removeFromTop (kPresetBarH));
    full.removeFromTop (kMargin);

    // Chain strip — left column, full remaining height
    chainStrip.setBounds (full.removeFromLeft (kChainW).withTrimmedLeft (kMargin));
    full.removeFromLeft (kMargin);

    // Right content area
    auto right = full.withTrimmedRight (kMargin);

    eqCurve.setBounds       (right.removeFromTop (kEQH));
    right.removeFromTop     (kMargin);
    eqControls.setBounds    (right.removeFromTop (kEQControlsH));
    right.removeFromTop     (kMargin);
    autotuneStrip.setBounds (right.removeFromTop (kAutoH));
    right.removeFromTop     (kMargin);

    // 2 × 3 panel grid
    std::array<juce::Component*, 6> panels {
        &deEsserPanel, &compressorPanel, &delayPanel,
        &saturationPanel, &harmonizerPanel, &reverbPanel
    };

    int rowH = (right.getHeight() - kMargin) / 2;
    int colW = (right.getWidth()  - kMargin * 2) / 3;
    int idx  = 0;

    for (int row = 0; row < 2; ++row)
    {
        auto rowArea = right.removeFromTop (rowH);
        if (row < 1) right.removeFromTop (kMargin);
        for (int col = 0; col < 3; ++col)
        {
            if (idx >= (int) panels.size()) break;
            auto cell = rowArea.removeFromLeft (colW);
            if (col < 2) rowArea.removeFromLeft (kMargin);
            panels[(size_t) idx++]->setBounds (cell);
        }
    }
}
