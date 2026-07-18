#include "CompressorPanel.h"

CompressorPanel::CompressorPanel (juce::AudioProcessorValueTreeState& apvts,
                                   CompressorModule* module)
    : ModulePanel ("COMPRESSOR", juce::Colour (0xffe07040)),
      compressor (module)
{
    makeKnob (thresholdSlider); makeLabel (thresholdLabel, "Threshold");
    makeKnob (ratioSlider);     makeLabel (ratioLabel,     "Ratio");
    makeKnob (attackSlider);    makeLabel (attackLabel,    "Attack");
    makeKnob (releaseSlider);   makeLabel (releaseLabel,   "Release");
    makeKnob (kneeSlider);      makeLabel (kneeLabel,      "Knee");
    makeKnob (makeupSlider);    makeLabel (makeupLabel,    "Makeup");

    for (auto* s : { &thresholdSlider, &ratioSlider, &attackSlider,
                     &releaseSlider, &kneeSlider, &makeupSlider })
        addAndMakeVisible (s);

    for (auto* l : { &thresholdLabel, &ratioLabel, &attackLabel,
                     &releaseLabel, &kneeLabel, &makeupLabel })
        addAndMakeVisible (l);

    thresholdAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "comp_threshold", thresholdSlider);
    ratioAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "comp_ratio",     ratioSlider);
    attackAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "comp_attack",    attackSlider);
    releaseAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "comp_release",   releaseSlider);
    kneeAtt      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "comp_knee",      kneeSlider);
    makeupAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "comp_makeup",    makeupSlider);
}

void CompressorPanel::resized()
{
    auto content = contentArea();

    // Visual row: transfer curve on left, GR meter on right
    auto vizRow   = content.removeFromTop (68);
    auto grArea   = vizRow.removeFromRight (22);
    auto curveArea = vizRow;
    (void) curveArea; (void) grArea; // used in paint()

    content.removeFromTop (4);

    // Two rows of three knobs
    auto row1 = content.removeFromTop (content.getHeight() / 2);
    auto row2 = content;

    layoutKnobRow (row1, { { &thresholdLabel, &thresholdSlider },
                            { &ratioLabel,     &ratioSlider     },
                            { &attackLabel,    &attackSlider    } });
    layoutKnobRow (row2, { { &releaseLabel,   &releaseSlider   },
                            { &kneeLabel,      &kneeSlider      },
                            { &makeupLabel,    &makeupSlider    } });
}

void CompressorPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);

    auto content  = contentArea();
    auto vizRow   = content.removeFromTop (68);
    auto grArea   = vizRow.removeFromRight (22).reduced (3, 6);
    auto curveArea = vizRow.reduced (3, 4);

    drawTransferCurve (g, curveArea);
    drawGrMeter       (g, grArea);
}

void CompressorPanel::drawTransferCurve (juce::Graphics& g, juce::Rectangle<int> area) const
{
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (area.toFloat(), 3.0f);

    auto w  = (float) area.getWidth();
    auto h  = (float) area.getHeight();
    auto x0 = (float) area.getX();
    auto y0 = (float) area.getY();

    // Axis labels
    g.setFont (juce::Font (juce::FontOptions().withHeight (8.0f)));
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawText ("In",  area.getRight() - 14, (int) (y0 + h) - 10, 14, 10, juce::Justification::centred);
    g.drawText ("Out", (int) x0,             (int) y0,             14, 10, juce::Justification::centredLeft);

    // 1:1 reference line
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawLine (x0, y0 + h, x0 + w, y0, 1.0f);

    // Read current params from sliders
    float threshold = (float) thresholdSlider.getValue();
    float ratio     = (float) ratioSlider.getValue();
    float knee      = (float) kneeSlider.getValue();
    float minDb = -60.0f, maxDb = 0.0f;

    // Draw the compressed transfer curve
    juce::Path curve;
    bool started = false;

    for (int px = 0; px < (int) w; px += 1)
    {
        float t       = (float) px / w;
        float inputDb = minDb + t * (maxDb - minDb);
        float over    = inputDb - threshold;

        float outputDb;
        if (knee > 0.0f && 2.0f * std::abs (over) <= knee)
        {
            float tt = over + knee * 0.5f;
            float gr = (1.0f / ratio - 1.0f) * tt * tt / (2.0f * knee);
            outputDb = inputDb + gr;
        }
        else if (over <= -knee * 0.5f)
        {
            outputDb = inputDb; // no compression
        }
        else
        {
            float gr = over * (1.0f / ratio - 1.0f);
            outputDb = inputDb + gr;
        }

        float py = y0 + h - ((outputDb - minDb) / (maxDb - minDb)) * h;
        py = juce::jlimit (y0, y0 + h, py);

        if (! started) { curve.startNewSubPath (x0 + (float) px, py); started = true; }
        else             curve.lineTo           (x0 + (float) px, py);
    }

    g.setColour (accentColour.withAlpha (0.85f));
    g.strokePath (curve, juce::PathStrokeType (1.5f));

    // Threshold tick on X axis
    float threshT = (threshold - minDb) / (maxDb - minDb);
    float threshX = x0 + threshT * w;
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawLine (threshX, y0, threshX, y0 + h, 0.5f);
}

void CompressorPanel::drawGrMeter (juce::Graphics& g, juce::Rectangle<int> area) const
{
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (area.toFloat(), 2.0f);

    float gr     = compressor != nullptr ? compressor->getCurrentGainReductionDb() : 0.0f;
    float maxGr  = 30.0f;
    float fill   = juce::jlimit (0.0f, 1.0f, -gr / maxGr);
    auto  filled = area.withTrimmedBottom ((int) ((1.0f - fill) * (float) area.getHeight()));

    // Gradient: yellow for light compression, red for heavy
    auto colour = fill < 0.5f
                  ? accentColour.interpolatedWith (juce::Colours::yellow, 0.4f)
                  : juce::Colours::orangered;
    g.setColour (colour.withAlpha (0.85f));
    g.fillRoundedRectangle (filled.toFloat(), 2.0f);

    g.setFont (juce::Font (juce::FontOptions().withHeight (8.0f)));
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.drawText ("GR", area.getX() - 2, area.getBottom() + 1, area.getWidth() + 4, 10,
                juce::Justification::centred);
}
