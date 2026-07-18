#include "HarmonizerPanel.h"

// ──────────────────────────────────────────── interval name lookup

juce::String HarmonizerPanel::intervalName (int semitones)
{
    switch (std::abs (semitones))
    {
        case 0:  return "Uni";
        case 1:  return "m2";
        case 2:  return "M2";
        case 3:  return "m3";
        case 4:  return "M3";
        case 5:  return "P4";
        case 6:  return "tt";
        case 7:  return "P5";
        case 8:  return "m6";
        case 9:  return "M6";
        case 10: return "m7";
        case 11: return "M7";
        case 12: return "Oct";
        default: return juce::String (semitones > 0 ? "+" : "") + juce::String (semitones);
    }
}

// ──────────────────────────────────────────── constructor

HarmonizerPanel::HarmonizerPanel (juce::AudioProcessorValueTreeState& apvts)
    : ModulePanel ("HARMONIZER", juce::Colour (0xff8050d0))
{
    makeKnob (int1Slider);      makeLabel (int1Label,      "Voice 1");
    makeKnob (int2Slider);      makeLabel (int2Label,      "Voice 2");
    makeKnob (mixSlider);       makeLabel (mixLabel,       "Mix");
    makeKnob (grainSlider);     makeLabel (grainLabel,     "Grain");
    makeKnob (humaniseSlider);  makeLabel (humaniseLabel,  "Humanise");
    makeKnob (widthSlider);     makeLabel (widthLabel,     "Width");

    for (auto* s : { &int1Slider, &int2Slider, &mixSlider, &grainSlider,
                     &humaniseSlider, &widthSlider })
        addAndMakeVisible (s);
    for (auto* l : { &int1Label, &int2Label, &mixLabel, &grainLabel,
                     &humaniseLabel, &widthLabel })
        addAndMakeVisible (l);

    int1Att      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "harm_interval1", int1Slider);
    int2Att      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "harm_interval2", int2Slider);
    mixAtt       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "harm_mix",       mixSlider);
    grainAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "harm_grain",     grainSlider);
    humaniseAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "harm_humanise",  humaniseSlider);
    widthAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "harm_width",     widthSlider);
}

// ──────────────────────────────────────────── layout

void HarmonizerPanel::resized()
{
    auto content = contentArea();

    content.removeFromTop (66);   // interval map — handled in paint()
    content.removeFromTop (4);

    // Row 1: Voice 1 interval, Voice 2 interval
    auto row1 = content.removeFromTop ((content.getHeight() - 4) / 2);
    layoutKnobRow (row1, { { &int1Label, &int1Slider },
                            { &int2Label, &int2Slider } });
    content.removeFromTop (4);

    // Row 2: Mix · Grain · Humanise · Width
    layoutKnobRow (content, { { &mixLabel,      &mixSlider      },
                               { &grainLabel,    &grainSlider    },
                               { &humaniseLabel, &humaniseSlider },
                               { &widthLabel,    &widthSlider    } });
}

// ──────────────────────────────────────────── paint

void HarmonizerPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);

    auto vizArea = contentArea().removeFromTop (66).reduced (4, 4);
    drawIntervalMap (g, vizArea);
}

// ──────────────────────────────────────────── interval map visual

void HarmonizerPanel::drawIntervalMap (juce::Graphics& g, juce::Rectangle<int> area) const
{
    auto fa = area.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (fa, 3.0f);

    // ── Axis: -12 to +12 semitones ───────────────────────────────────────
    constexpr float stMin = -12.0f, stMax = 12.0f;

    auto stToX = [&] (float st) -> float {
        return fa.getX() + (st - stMin) / (stMax - stMin) * fa.getWidth();
    };

    float axisY = fa.getY() + fa.getHeight() * 0.68f;   // axis sits 68% down

    // Semitone tick marks
    g.setFont (juce::Font (juce::FontOptions().withHeight (7.5f)));
    for (int st = (int) stMin; st <= (int) stMax; st += 1)
    {
        float x = stToX ((float) st);
        bool  isOct = (st == 0 || std::abs (st) == 12);
        bool  labelled = (st % 3 == 0);

        g.setColour (juce::Colours::white.withAlpha (isOct ? 0.4f : 0.12f));
        float tickH = isOct ? 6.0f : (labelled ? 4.0f : 2.5f);
        g.drawLine (x, axisY - tickH, x, axisY + tickH, isOct ? 1.0f : 0.5f);

        if (labelled && st != 0)
        {
            g.setColour (juce::Colours::white.withAlpha (0.25f));
            g.drawText (juce::String (st), (int) x - 8, (int) axisY + 5, 16, 9,
                        juce::Justification::centred);
        }
    }

    // Axis line
    g.setColour (juce::Colours::white.withAlpha (0.2f));
    g.drawLine (fa.getX(), axisY, fa.getRight(), axisY, 0.5f);

    // ── Dry marker (0 st) ─────────────────────────────────────────────────
    {
        float x = stToX (0.0f);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.fillRect (x - 0.75f, axisY - 8.0f, 1.5f, 16.0f);
        g.setFont (juce::Font (juce::FontOptions().withHeight (7.5f)));
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawText ("dry", (int) x - 10, (int) fa.getY(), 20, 10,
                    juce::Justification::centred);
    }

    // ── Voice markers ─────────────────────────────────────────────────────
    const juce::Colour voiceColours[2] = {
        juce::Colour (0xff60b0ff),
        juce::Colour (0xffe070ff)
    };
    const float intervals[2] = {
        (float) int1Slider.getValue(),
        (float) int2Slider.getValue()
    };

    g.setFont (juce::Font (juce::FontOptions().withHeight (8.5f).withStyle ("Bold")));

    for (int v = 0; v < 2; ++v)
    {
        float st = juce::jlimit (stMin, stMax, intervals[v]);
        float x  = stToX (st);
        auto  col = voiceColours[v];

        // Connecting line from axis to marker
        float markerY = v == 0 ? fa.getY() + 2.0f : axisY - 18.0f;
        g.setColour (col.withAlpha (0.35f));
        g.drawLine (x, axisY, x, markerY + 8.0f, 1.0f);

        // Pill-shaped marker
        juce::Rectangle<float> pill (x - 14.0f, markerY, 28.0f, 14.0f);
        g.setColour (col.withAlpha (0.8f));
        g.fillRoundedRectangle (pill, 4.0f);

        // Interval name inside pill
        g.setColour (juce::Colours::white);
        auto name = intervalName (juce::roundToInt (intervals[v]));
        g.drawText (name, pill.toNearestInt(), juce::Justification::centred);

        // Semitone label below pill (for non-standard intervals)
        if (std::abs (juce::roundToInt (intervals[v])) > 12)
        {
            g.setFont (juce::Font (juce::FontOptions().withHeight (7.5f)));
            g.setColour (col.withAlpha (0.6f));
            auto stStr = (intervals[v] > 0 ? "+" : "") + juce::String (juce::roundToInt (intervals[v])) + "st";
            g.drawText (stStr, (int) x - 14, (int) markerY + 14, 28, 9, juce::Justification::centred);
        }
    }
}
