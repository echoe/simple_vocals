#include "DelayPanel.h"

DelayPanel::DelayPanel (juce::AudioProcessorValueTreeState& apvts)
    : ModulePanel ("DELAY", juce::Colour (0xff60a8e0))
{
    makeKnob (timeSlider);     makeLabel (timeLabel,     "Time");
    makeKnob (feedbackSlider); makeLabel (feedbackLabel, "Feedback");
    makeKnob (mixSlider);      makeLabel (mixLabel,      "Mix");
    makeKnob (toneSlider);     makeLabel (toneLabel,     "Tone");

    for (auto* s : { &timeSlider, &feedbackSlider, &mixSlider, &toneSlider })
        addAndMakeVisible (s);
    for (auto* l : { &timeLabel, &feedbackLabel, &mixLabel, &toneLabel })
        addAndMakeVisible (l);

    pingPongButton.setButtonText ("Ping-Pong");
    addAndMakeVisible (pingPongButton);

    timeAtt      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "dly_time",     timeSlider);
    feedbackAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "dly_feedback", feedbackSlider);
    mixAtt       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "dly_mix",      mixSlider);
    toneAtt      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "dly_tone",     toneSlider);
    pingPongAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, "dly_pingpong", pingPongButton);
}

void DelayPanel::resized()
{
    auto content = contentArea();

    content.removeFromTop (68);  // echo viz
    content.removeFromTop (4);

    // Row 1: Time, Feedback
    auto row1 = content.removeFromTop ((content.getHeight() - 4) / 2);
    content.removeFromTop (4);
    layoutKnobRow (row1, { { &timeLabel,     &timeSlider     },
                            { &feedbackLabel, &feedbackSlider } });

    // Row 2: Mix knob · Tone knob · Ping-Pong toggle (inline like Freeze in Reverb)
    auto row2 = content;
    auto ppCell = row2.removeFromRight (row2.getWidth() / 3);
    layoutKnobRow (row2, { { &mixLabel,  &mixSlider  },
                            { &toneLabel, &toneSlider } });
    pingPongButton.setBounds (ppCell.reduced (6, (ppCell.getHeight() - 22) / 2));
}

void DelayPanel::paint (juce::Graphics& g)
{
    ModulePanel::paint (g);
    auto vizArea = contentArea().removeFromTop (68).reduced (3, 4);
    drawEchoViz (g, vizArea);
}

void DelayPanel::drawEchoViz (juce::Graphics& g, juce::Rectangle<int> area) const
{
    auto fa = area.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (fa, 3.0f);

    float timeMs    = (float) timeSlider.getValue();
    float feedback  = (float) feedbackSlider.getValue();
    float mix       = (float) mixSlider.getValue();
    bool  pingPong  = pingPongButton.getToggleState();

    // Display window: show at least 4 taps or up to 2s
    constexpr float kMaxDisplayMs = 2000.0f;
    float windowMs = juce::jmin (kMaxDisplayMs, timeMs * 5.0f);

    // Axis
    g.setFont (juce::Font (juce::FontOptions().withHeight (8.0f)));
    for (float t : { 250.0f, 500.0f, 1000.0f, 1500.0f, 2000.0f })
    {
        if (t > windowMs) break;
        float x = fa.getX() + (t / windowMs) * fa.getWidth();
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawLine (x, fa.getY(), x, fa.getBottom() - 10.0f, 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.2f));
        auto label = t >= 1000.0f ? juce::String (t / 1000.0f, 1) + "s" : juce::String ((int)t) + "ms";
        g.drawText (label, (int)x - 14, (int)fa.getBottom() - 10, 28, 9, juce::Justification::centred);
    }

    // Dry impulse at t=0
    float dryH = (fa.getHeight() - 12.0f) * 0.85f;
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.fillRect (fa.getX() + 1.0f, fa.getBottom() - 12.0f - dryH, 3.0f, dryH);

    // Echo taps
    float amp = mix;
    float pingSign = 1.0f;  // +1 = left, -1 = right (for ping-pong colouring)
    for (int tap = 1; tap <= 8 && amp > 0.01f; ++tap)
    {
        float tapMs = timeMs * (float) tap;
        if (tapMs > windowMs) break;

        float x    = fa.getX() + (tapMs / windowMs) * fa.getWidth();
        float tapH = (fa.getHeight() - 12.0f) * juce::jlimit (0.0f, 1.0f, amp);

        juce::Colour tapCol = pingPong
            ? (pingSign > 0 ? juce::Colour (0xff60b0ff) : juce::Colour (0xffe070ff))
            : accentColour;
        g.setColour (tapCol.withAlpha (0.8f));
        g.fillRect (x - 1.5f, fa.getBottom() - 12.0f - tapH, 3.0f, tapH);

        amp      *= feedback;
        pingSign  = -pingSign;
    }
}
