#include "SaturationPanel.h"

// ──────────────────────────────────────────────────────────────── helpers

namespace
{
    const juce::StringArray kModeNames { "Tape", "Tube", "Clip", "Foldback" };

    /** Read the current mode index from the APVTS parameter (0–3). Works
        regardless of whether JUCE stores it normalised or denormalised. */
    int readModeIndex (juce::AudioProcessorValueTreeState& apvts)
    {
        if (auto* p = apvts.getParameter ("sat_mode"))
            return juce::roundToInt (p->convertFrom0to1 (p->getValue()));
        return 0;
    }

    /** Push a mode index back to the APVTS parameter. */
    void writeModeIndex (juce::AudioProcessorValueTreeState& apvts, int idx)
    {
        if (auto* p = apvts.getParameter ("sat_mode"))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 ((float) idx));
            p->endChangeGesture();
        }
    }
}

// ──────────────────────────────────────────────────────────────── constructor

SaturationPanel::SaturationPanel (juce::AudioProcessorValueTreeState& a)
    : ModulePanel ("SATURATION", juce::Colour (0xffd06040)),
      apvts (a)
{
    // ── Mode combo ─────────────────────────────────────────────────────
    modeLabel.setText ("Mode", juce::dontSendNotification);
    modeLabel.setJustificationType (juce::Justification::centredLeft);
    modeLabel.setFont (juce::Font (juce::FontOptions().withHeight (13.0f)));
    modeLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.55f));

    // Populate manually — avoids ComboBoxAttachment version-dependency
    for (int i = 0; i < kModeNames.size(); ++i)
        modeBox.addItem (kModeNames[i], i + 1);   // item IDs are 1-based in JUCE

    modeBox.setSelectedItemIndex (readModeIndex (apvts), juce::dontSendNotification);
    modeBox.setJustificationType (juce::Justification::centred);
    modeBox.onChange = [this]
    {
        writeModeIndex (apvts, modeBox.getSelectedItemIndex());
    };

    addAndMakeVisible (modeLabel);
    addAndMakeVisible (modeBox);

    // ── Knobs ──────────────────────────────────────────────────────────
    makeKnob (driveSlider);  makeLabel (driveLabel,  "Drive");
    makeKnob (toneSlider);   makeLabel (toneLabel,   "Tone");
    makeKnob (mixSlider);    makeLabel (mixLabel,    "Mix");
    makeKnob (outputSlider); makeLabel (outputLabel, "Output");

    for (auto* s : { &driveSlider, &toneSlider, &mixSlider, &outputSlider })
        addAndMakeVisible (s);
    for (auto* l : { &driveLabel, &toneLabel, &mixLabel, &outputLabel })
        addAndMakeVisible (l);

    driveAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "sat_drive",  driveSlider);
    toneAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "sat_tone",   toneSlider);
    mixAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "sat_mix",    mixSlider);
    outputAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "sat_output", outputSlider);
}

// ──────────────────────────────────────────────────────────────── timer

void SaturationPanel::timerCallback()
{
    // Keep the combo in sync with the parameter (needed if host automates it)
    int paramIdx = readModeIndex (apvts);
    if (modeBox.getSelectedItemIndex() != paramIdx)
        modeBox.setSelectedItemIndex (paramIdx, juce::dontSendNotification);

    repaint();
}

// ──────────────────────────────────────────────────────────────── layout

void SaturationPanel::resized()
{
    auto content = contentArea();

    content.removeFromTop (72);   // transfer curve — handled in paint()
    content.removeFromTop (4);

    auto modeRow = content.removeFromTop (22);
    modeLabel.setBounds (modeRow.removeFromLeft (48));
    modeRow.removeFromLeft (4);
    modeBox.setBounds (modeRow);

    content.removeFromTop (5);

    layoutKnobRow (content, { { &driveLabel,  &driveSlider  },
                               { &toneLabel,   &toneSlider   },
                               { &mixLabel,    &mixSlider    },
                               { &outputLabel, &outputSlider } });
}

// ──────────────────────────────────────────────────────────────── paint

void SaturationPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);

    auto curveArea = contentArea().removeFromTop (72).reduced (3, 4);
    drawTransferCurve (g, curveArea);
}

// ──────────────────────────────────────────────────────────────── transfer curve

void SaturationPanel::drawTransferCurve (juce::Graphics& g, juce::Rectangle<int> area) const
{
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (area.toFloat(), 3.0f);

    auto fa = area.toFloat();
    auto w  = fa.getWidth();
    auto h  = fa.getHeight();
    auto x0 = fa.getX();
    auto y0 = fa.getY();
    auto cx = x0 + w * 0.5f;
    auto cy = y0 + h * 0.5f;

    // Grid
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine (cx, y0, cx, y0 + h, 0.5f);
    g.drawLine (x0, cy, x0 + w, cy, 0.5f);

    // 1:1 reference line
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawLine (x0, y0 + h, x0 + w, y0, 1.0f);

    float drive = (float) driveSlider.getValue();
    int   mode  = modeBox.getSelectedItemIndex();
    if (mode < 0) mode = 0;

    // Mode label
    g.setFont (juce::Font (juce::FontOptions().withHeight (8.5f)));
    g.setColour (accentColour.withAlpha (0.6f));
    if (mode < kModeNames.size())
        g.drawText (kModeNames[mode], area.reduced (3), juce::Justification::topRight);

    // Transfer curve
    juce::Path curve;
    bool started = false;

    for (int px = 0; px < (int) w; ++px)
    {
        float xNorm = (float) px / w * 2.0f - 1.0f;          // -1 → +1
        float yNorm = juce::jlimit (-1.0f, 1.0f,
                                    SaturationModule::waveshapeStatic (xNorm, drive, mode));

        float drawX = x0 + (xNorm + 1.0f) * 0.5f * w;
        float drawY = y0 + h - (yNorm + 1.0f) * 0.5f * h;

        if (! started) { curve.startNewSubPath (drawX, drawY); started = true; }
        else             curve.lineTo (drawX, drawY);
    }

    g.setColour (accentColour.withAlpha (0.9f));
    g.strokePath (curve, juce::PathStrokeType (1.8f));

    // Axis tick labels (±1)
    g.setFont (juce::Font (juce::FontOptions().withHeight (8.0f)));
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawText ("-1", (int) x0,              (int) cy - 9, 16, 10, juce::Justification::centredLeft);
    g.drawText ("+1", (int) (x0 + w) - 16,  (int) cy - 9, 16, 10, juce::Justification::centredRight);
}
