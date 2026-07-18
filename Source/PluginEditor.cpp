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

    constexpr int rightColW  = 620;   // 3 x 2 grid of module panels
    constexpr int panelRowH  = 220;

    int leftColH  = kEQH + kMargin + kEQControlsH + kMargin + kAutoH;
    int rightColH = panelRowH * 2 + kMargin;
    int bodyH     = std::max (leftColH, rightColH);

    int totalW = kMargin + kLeftColW + kMargin + rightColW + kMargin;
    int totalH = kPresetBarH + kMargin + kChainH + kMargin + bodyH + kMargin;

    setSize (totalW, totalH);
}

void SimpleVocalsAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff14141a));

    // Background panel behind the chain strip
    g.setColour (juce::Colours::white.withAlpha (0.03f));
    g.fillRoundedRectangle (
        juce::Rectangle<int> (kMargin, kPresetBarH + kMargin,
                              getWidth() - kMargin * 2, kChainH).toFloat(), 4.0f);

    // Separator below the chain strip
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    int sepY = kPresetBarH + kMargin + kChainH + kMargin / 2;
    g.drawLine ((float) kMargin, (float) sepY,
                (float) (getWidth() - kMargin), (float) sepY, 1.0f);
}

void SimpleVocalsAudioProcessorEditor::resized()
{
    auto full = getLocalBounds();

    // Preset bar — full width at top
    presetBar.setBounds (full.removeFromTop (kPresetBarH));
    full.removeFromTop (kMargin);

    // Chain strip — full width, runs left to right
    chainStrip.setBounds (full.removeFromTop (kChainH));
    full.removeFromTop (kMargin);

    // Left column: EQ curve, EQ band controls, Autotune (narrowed)
    auto left = full.removeFromLeft (kLeftColW);
    full.removeFromLeft (kMargin);

    eqCurve.setBounds       (left.removeFromTop (kEQH));
    left.removeFromTop      (kMargin);
    eqControls.setBounds    (left.removeFromTop (kEQControlsH));
    left.removeFromTop      (kMargin);
    autotuneStrip.setBounds (left.removeFromTop (kAutoH));

    // Right column: 2 x 3 grid of module panels
    auto right = full;

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
