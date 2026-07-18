#include "DeEsserPanel.h"

DeEsserPanel::DeEsserPanel (juce::AudioProcessorValueTreeState& apvts, DeEsserModule* module)
    : ModulePanel ("DE-ESSER", juce::Colour (0xff20b8a0)),
      deEsser (module)
{
    makeKnob (freqSlider);      makeLabel (freqLabel,      "Freq");
    makeKnob (thresholdSlider); makeLabel (thresholdLabel, "Threshold");
    makeKnob (rangeSlider);     makeLabel (rangeLabel,     "Range dB");

    addAndMakeVisible (freqSlider);
    addAndMakeVisible (thresholdSlider);
    addAndMakeVisible (rangeSlider);
    addAndMakeVisible (freqLabel);
    addAndMakeVisible (thresholdLabel);
    addAndMakeVisible (rangeLabel);

    freqAtt      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "deess_freq",      freqSlider);
    thresholdAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "deess_threshold", thresholdSlider);
    rangeAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "deess_range",     rangeSlider);
}

void DeEsserPanel::resized()
{
    auto content = contentArea();

    // Top: visualisations (freq display left, GR meter right)
    auto vizRow = content.removeFromTop (68);
    auto grArea   = vizRow.removeFromRight (28);
    auto freqArea = vizRow;
    // Store for paint — layout in paint() so it redraws correctly
    (void) freqArea; (void) grArea;

    content.removeFromTop (6); // gap

    // Knob row fills the rest
    layoutKnobRow (content, { { &freqLabel,      &freqSlider      },
                               { &thresholdLabel, &thresholdSlider },
                               { &rangeLabel,     &rangeSlider     } });
}

void DeEsserPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);

    auto content = contentArea();
    auto vizRow  = content.removeFromTop (68);
    auto grArea  = vizRow.removeFromRight (28).reduced (4, 6);
    auto freqArea = vizRow.reduced (2, 4);

    drawFreqViz (g, freqArea);
    drawGrMeter (g, grArea);
}

void DeEsserPanel::drawFreqViz (juce::Graphics& g, juce::Rectangle<int> area) const
{
    // Background
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.fillRoundedRectangle (area.toFloat(), 3.0f);

    // Frequency range: 1kHz – 20kHz, log scale
    auto w = (float) area.getWidth();
    auto h = (float) area.getHeight();
    auto x0 = (float) area.getX();
    auto y0 = (float) area.getY();

    float logMin = std::log10 (1000.0f), logMax = std::log10 (20000.0f);

    // Crossover frequency from slider
    float crossFreq = (float) freqSlider.getValue();
    float crossT = (std::log10 (juce::jlimit (1000.0f, 20000.0f, crossFreq)) - logMin) / (logMax - logMin);
    float crossX = x0 + crossT * w;

    // Low band (grey)
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRect (x0, y0, crossX - x0, h);

    // Sibilant zone (teal, above crossover)
    g.setColour (accentColour.withAlpha (0.18f));
    g.fillRect (crossX, y0, x0 + w - crossX, h);

    // Crossover line
    g.setColour (accentColour.withAlpha (0.7f));
    g.drawLine (crossX, y0, crossX, y0 + h, 1.5f);

    // Freq label
    auto freqStr = crossFreq >= 1000.0f
                   ? juce::String (crossFreq / 1000.0f, 1) + "k"
                   : juce::String ((int) crossFreq);
    g.setFont (juce::Font (juce::FontOptions().withHeight (9.0f)));
    g.setColour (accentColour);
    g.drawText (freqStr + " Hz", area.reduced (2), juce::Justification::bottomLeft);

    // Freq tick labels (1k, 5k, 10k, 20k)
    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.setFont (juce::Font (juce::FontOptions().withHeight (8.0f)));
    for (auto [f, label] : { std::pair { 1000.0f, "1k" }, { 5000.0f, "5k" }, { 10000.0f, "10k" } })
    {
        auto t = (std::log10 (f) - logMin) / (logMax - logMin);
        auto lx = (int) (x0 + t * w);
        g.drawLine ((float) lx, y0 + h - 6.0f, (float) lx, y0 + h, 0.5f);
        g.drawText (label, lx - 8, (int) (y0 + h) - 10, 16, 10, juce::Justification::centred);
    }
}

void DeEsserPanel::drawGrMeter (juce::Graphics& g, juce::Rectangle<int> area) const
{
    // Vertical bar: top = 0dB GR, bottom = max reduction
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (area.toFloat(), 2.0f);

    float gr = deEsser != nullptr ? deEsser->getCurrentGainReductionDb() : 0.0f;
    float maxRange = (float) rangeSlider.getValue();
    if (maxRange < 0.01f) maxRange = 24.0f;

    float fill = juce::jlimit (0.0f, 1.0f, -gr / maxRange);
    auto fillRect = area.withTrimmedBottom ((int) ((1.0f - fill) * (float) area.getHeight()));

    g.setColour (accentColour.withAlpha (0.85f));
    g.fillRoundedRectangle (fillRect.toFloat(), 2.0f);

    // "GR" label
    g.setFont (juce::Font (juce::FontOptions().withHeight (8.0f)));
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawText ("GR", area.getX(), area.getBottom() + 1, area.getWidth(), 10,
                juce::Justification::centred);
}
