#include "PluginEditor.h"

SimpleVocalsAudioProcessorEditor::SimpleVocalsAudioProcessorEditor (SimpleVocalsAudioProcessor& p)
    : AudioProcessorEditor (&p),
      presetBar       (p.presetManager),
      autoChainButton (p, p.apvts, p.effectChain),
      chainStrip      (p.effectChain, p.apvts),
      eqCurve1        (p.apvts, "eq"),
      eqCurve2        (p.apvts, "eq2"),
      eqControls1     (p.apvts, "eq"),
      eqControls2     (p.apvts, "eq2"),
      autotuneStrip   (p.apvts, p.effectChain.getModuleOfType<AutotuneModule>()),
      deEsserPanel      (p.apvts, p.effectChain.getModuleOfType<DeEsserModule>()),
      compressorPanel   (p.apvts, p.effectChain.getModuleOfType<CompressorModule>()),
      delayPanel        (p.apvts),
      saturationPanel   (p.apvts),
      reverbPanel       (p.apvts),
      denoisePanel      (p.apvts, p.effectChain.getModuleOfType<NoiseReductionModule>()),
      pitchFormantPanel (p.apvts)
{
    addAndMakeVisible (presetBar);
    addAndMakeVisible (autoChainButton);
    addAndMakeVisible (chainStrip);

    eq1TabButton.setClickingTogglesState (false);
    eq2TabButton.setClickingTogglesState (false);
    eq1TabButton.onClick = [this] { selectEqTab (0); };
    eq2TabButton.onClick = [this] { selectEqTab (1); };
    addAndMakeVisible (eq1TabButton);
    addAndMakeVisible (eq2TabButton);

    addAndMakeVisible (eqCurve1);
    addAndMakeVisible (eqCurve2);
    addAndMakeVisible (eqControls1);
    addAndMakeVisible (eqControls2);

    addAndMakeVisible (autotuneStrip);
    addAndMakeVisible (deEsserPanel);
    addAndMakeVisible (compressorPanel);
    addAndMakeVisible (delayPanel);
    addAndMakeVisible (saturationPanel);
    addAndMakeVisible (reverbPanel);
    addAndMakeVisible (denoisePanel);
    addAndMakeVisible (pitchFormantPanel);

    selectEqTab (0);

    constexpr int rightColW = 620;   // 2 x 3 grid of module panels, no gaps between cells

    // Force the body height to perfectly match the 3 right-side modules
    int bodyH = kFullRowH * 3;

    int totalW = kMargin + kLeftColW + rightColW + kMargin;   // no gap between the two columns
    int totalH = kPresetBarH + kMargin + kChainH + kMargin + bodyH + kMargin;

    setSize (totalW, totalH);
}

void SimpleVocalsAudioProcessorEditor::selectEqTab (int tab)
{
    selectedEqTab = tab;

    eqCurve1.setVisible    (tab == 0);
    eqControls1.setVisible (tab == 0);
    eqCurve2.setVisible    (tab == 1);
    eqControls2.setVisible (tab == 1);

    eq1TabButton.setColour (juce::TextButton::buttonColourId,
                             tab == 0 ? juce::Colour (0xff4466cc) : juce::Colour (0xff2a2a3a));
    eq2TabButton.setColour (juce::TextButton::buttonColourId,
                             tab == 1 ? juce::Colour (0xff4466cc) : juce::Colour (0xff2a2a3a));
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

    // Preset bar — full width at top, with the Auto Chain button on the right
    auto topRow = full.removeFromTop (kPresetBarH);
    autoChainButton.setBounds (topRow.removeFromRight (110));
    topRow.removeFromRight (kMargin);
    presetBar.setBounds (topRow);
    full.removeFromTop (kMargin);

    // Chain strip — full width, runs left to right
    chainStrip.setBounds (full.removeFromTop (kChainH));
    full.removeFromTop (kMargin);

    // --- LEFT COLUMN ALIGNMENT FIX ---
    auto left = full.removeFromLeft (kLeftColW);

    // 1. Give Autotune exactly the height of one right-side module at the bottom
    autotuneStrip.setBounds (left.removeFromBottom (kFullRowH));

    // 2. Lay out EQ Tabs from the top
    auto tabRow = left.removeFromTop (kEQTabH);
    int  tabW   = tabRow.getWidth() / 2;
    eq1TabButton.setBounds (tabRow.removeFromLeft (tabW).reduced (1));
    eq2TabButton.setBounds (tabRow.reduced (1));

    // 3. Lay out EQ Controls right above the Autotune
    auto eqControlsArea = left.removeFromBottom (kEQControlsH);
    eqControls1.setBounds (eqControlsArea);
    eqControls2.setBounds (eqControlsArea);

    // 4. The EQ Curve naturally fills whatever space is left, perfectly aligning it
    eqCurve1.setBounds (left);
    eqCurve2.setBounds (left);


    // --- RIGHT COLUMN ---
    auto right = full;
    int  colW  = right.getWidth() / 2;

    auto placeRow = [&] (juce::Rectangle<int> row, juce::Component& leftComp, juce::Component& rightComp)
    {
        auto cellL = row.removeFromLeft (colW);
        leftComp.setBounds (cellL);
        rightComp.setBounds (row);
    };

    auto row0 = right.removeFromTop (kFullRowH);
    placeRow (row0, deEsserPanel, compressorPanel);

    auto row1 = right.removeFromTop (kFullRowH);
    placeRow (row1, delayPanel, saturationPanel);

    auto row2 = right;   // remaining space — should equal kFullRowH
    auto reverbCell = row2.removeFromLeft (colW);
    reverbPanel.setBounds (reverbCell);

    denoisePanel.setBounds      (row2.removeFromTop (kHalfRowH));
    pitchFormantPanel.setBounds (row2);
}
