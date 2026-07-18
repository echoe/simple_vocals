#include "ReverbPanel.h"

ReverbPanel::ReverbPanel (juce::AudioProcessorValueTreeState& apvts)
    : ModulePanel ("REVERB", juce::Colour (0xff4080d0))
{
    makeKnob (sizeSlider);      makeLabel (sizeLabel,     "Size");
    makeKnob (dampingSlider);   makeLabel (dampingLabel,  "Damping");
    makeKnob (widthSlider);     makeLabel (widthLabel,    "Width");
    makeKnob (predelaySlider);  makeLabel (predelayLabel, "Pre-dly");
    makeKnob (mixSlider);       makeLabel (mixLabel,      "Mix");

    for (auto* s : { &sizeSlider, &dampingSlider, &widthSlider,
                     &predelaySlider, &mixSlider })
        addAndMakeVisible (s);
    for (auto* l : { &sizeLabel, &dampingLabel, &widthLabel,
                     &predelayLabel, &mixLabel })
        addAndMakeVisible (l);

    freezeButton.setButtonText ("Freeze");
    addAndMakeVisible (freezeButton);

    sizeAtt      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "verb_size",      sizeSlider);
    dampingAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "verb_damping",   dampingSlider);
    widthAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "verb_width",     widthSlider);
    predelayAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "verb_predelay",  predelaySlider);
    mixAtt       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "verb_mix",       mixSlider);
    freezeAtt    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, "verb_freeze",    freezeButton);
}

void ReverbPanel::resized()
{
    auto content = contentArea();

    content.removeFromTop (68);   // decay visual — handled in paint()
    content.removeFromTop (4);

    // Row 1: Size, Damping, Width
    auto row1 = content.removeFromTop ((content.getHeight() - 4) / 2);
    layoutKnobRow (row1, { { &sizeLabel,    &sizeSlider    },
                            { &dampingLabel, &dampingSlider },
                            { &widthLabel,   &widthSlider   } });
    content.removeFromTop (4);

    // Row 2: Pre-delay knob · Mix knob · Freeze toggle
    auto row2 = content;
    auto freezeCell = row2.removeFromRight (row2.getWidth() / 3);
    layoutKnobRow (row2, { { &predelayLabel, &predelaySlider },
                            { &mixLabel,      &mixSlider      } });

    // Freeze button — centred vertically in its cell
    freezeButton.setBounds (freezeCell.reduced (6, (freezeCell.getHeight() - 22) / 2));
}

void ReverbPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);

    auto vizArea = contentArea().removeFromTop (68).reduced (3, 4);
    drawDecayTail (g, vizArea);
}

void ReverbPanel::drawDecayTail (juce::Graphics& g, juce::Rectangle<int> area) const
{
    auto fa = area.toFloat();
    auto w = fa.getWidth();
    auto h = fa.getHeight();
    auto x0 = fa.getX();
    auto y0 = fa.getY();

    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (fa, 3.0f);

    // ── Parameter reads (from attached sliders) ───────────────────────────
    float size      = (float) sizeSlider.getValue();
    float damping   = (float) dampingSlider.getValue();
    float predelayMs = (float) predelaySlider.getValue();
    float mix       = (float) mixSlider.getValue();
    bool  frozen    = freezeButton.getToggleState();

    // Estimate RT60: higher size → longer tail, higher damping → shorter tail
    // Very rough approximation based on Freeverb's general character.
    float rt60 = 0.3f + size * 4.5f * (1.0f - damping * 0.75f);
    rt60 = juce::jlimit (0.1f, 6.0f, rt60);

    // Total display window: predelay + rt60 + 20% headroom, capped at 6s
    float windowSec = juce::jlimit (0.5f, 6.0f, predelayMs / 1000.0f + rt60 * 1.2f);

    // ── Grid lines ────────────────────────────────────────────────────────
    g.setFont (juce::Font (juce::FontOptions().withHeight (8.0f)));

    float tickIntervalSec = windowSec > 3.0f ? 1.0f : 0.5f;
    for (float t = 0.0f; t < windowSec; t += tickIntervalSec)
    {
        float tx = x0 + (t / windowSec) * w;
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawLine (tx, y0, tx, y0 + h, 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        auto label = t < 1.0f ? juce::String ((int) (t * 1000)) + "ms"
                               : juce::String (t, 1) + "s";
        g.drawText (label, (int) tx - 14, (int) (y0 + h) - 11, 28, 10,
                    juce::Justification::centred);
    }

    // ── Pre-delay marker ─────────────────────────────────────────────────
    float preDelayX = x0 + (predelayMs / 1000.0f / windowSec) * w;
    if (predelayMs > 1.0f)
    {
        g.setColour (accentColour.withAlpha (0.4f));
        g.drawLine (preDelayX, y0, preDelayX, y0 + h - 12.0f, 1.0f);
        g.setFont (juce::Font (juce::FontOptions().withHeight (8.0f)));
        g.drawText (juce::String ((int) predelayMs) + "ms",
                    (int) preDelayX - 14, (int) y0, 28, 10, juce::Justification::centred);
    }

    // ── Decay envelope ────────────────────────────────────────────────────
    juce::Path envelope;
    bool started = false;

    float decayRate = frozen ? 0.0f : (std::log (1000.0f) / rt60); // -60dB at rt60 seconds

    for (int px = 0; px < (int) w; ++px)
    {
        float t = ((float) px / w) * windowSec;

        float amp;
        if (t < predelayMs / 1000.0f)
        {
            amp = 0.0f; // silent during pre-delay
        }
        else
        {
            float tIntoTail = t - predelayMs / 1000.0f;
            amp = frozen ? 1.0f : std::exp (-decayRate * tIntoTail);
        }

        amp *= mix; // scale by wet mix so visual reflects actual level

        float py = y0 + h - 12.0f - amp * (h - 14.0f);
        float fx = x0 + (float) px;

        if (! started) { envelope.startNewSubPath (fx, py); started = true; }
        else             envelope.lineTo (fx, py);
    }

    // Filled area under the envelope
    if (started)
    {
        envelope.lineTo (x0 + w, y0 + h - 12.0f);
        envelope.lineTo (x0,     y0 + h - 12.0f);
        envelope.closeSubPath();
    }

    g.setColour (accentColour.withAlpha (0.18f));
    g.fillPath (envelope);

    // Envelope outline
    juce::Path outline;
    started = false;
    for (int px = 0; px < (int) w; ++px)
    {
        float t = ((float) px / w) * windowSec;
        float amp;
        if (t < predelayMs / 1000.0f)       amp = 0.0f;
        else                                 amp = frozen ? 1.0f
                                                           : std::exp (-decayRate * (t - predelayMs / 1000.0f));
        amp *= mix;

        float py = y0 + h - 12.0f - amp * (h - 14.0f);
        float fx = x0 + (float) px;
        if (! started) { outline.startNewSubPath (fx, py); started = true; }
        else             outline.lineTo (fx, py);
    }

    g.setColour (accentColour.withAlpha (0.85f));
    g.strokePath (outline, juce::PathStrokeType (1.5f));

    // RT60 annotation
    float rt60x = x0 + ((predelayMs / 1000.0f + rt60) / windowSec) * w;
    if (rt60x < x0 + w - 2.0f && ! frozen)
    {
        g.setColour (juce::Colours::white.withAlpha (0.2f));
        g.drawLine (rt60x, y0 + 2.0f, rt60x, y0 + h - 12.0f, 0.5f);
        g.setFont (juce::Font (juce::FontOptions().withHeight (8.0f)));
        g.setColour (juce::Colours::white.withAlpha (0.3f));
        auto rt60Label = "RT60 " + (rt60 < 1.0f ? juce::String ((int)(rt60 * 1000)) + "ms"
                                                 : juce::String (rt60, 1) + "s");
        g.drawText (rt60Label, (int) rt60x - 24, (int) y0, 48, 10,
                    juce::Justification::centred);
    }

    if (frozen)
    {
        g.setFont (juce::Font (juce::FontOptions().withHeight (9.0f).withStyle ("Bold")));
        g.setColour (accentColour.withAlpha (0.8f));
        g.drawText ("FROZEN", area.reduced (4), juce::Justification::topRight);
    }
}
