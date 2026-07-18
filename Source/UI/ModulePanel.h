#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

/** Base class for a module's control panel. Provides a consistent
    background, title bar, border, and a 30Hz repaint timer (needed by
    subclasses that display live GR meters). Subclasses add their own
    child components and implement resized(). */
class ModulePanel : public juce::Component,
                    private juce::Timer
{
public:
    ModulePanel (const juce::String& title, juce::Colour accent)
        : accentColour (accent), panelTitle (title)
    {
        startTimerHz (30);
    }

    ~ModulePanel() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        // Panel background
        g.setColour (juce::Colour (0xff1e1e24));
        g.fillRoundedRectangle (b, 6.0f);

        // Title bar background
        g.setColour (accentColour.withAlpha (0.22f));
        g.fillRoundedRectangle (b.removeFromTop (24.0f), 6.0f);
        g.setColour (accentColour.withAlpha (0.22f));
        g.fillRect (juce::Rectangle<float> (0.0f, 12.0f, b.getWidth(), 12.0f));

        // Title text
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.setFont (juce::Font (juce::FontOptions().withHeight (11.0f).withStyle ("Bold")));
        g.drawText (panelTitle, 8, 0, getWidth() - 16, 24, juce::Justification::centredLeft);

        // Border
        g.setColour (accentColour.withAlpha (0.4f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
    }

    /** Area available for controls, below the title bar. */
    juce::Rectangle<int> contentArea() const
    {
        return getLocalBounds().withTrimmedTop (26).reduced (6, 4);
    }

    juce::Colour accentColour;

protected:
    /** Subclasses can override to do their own repaint work; always call base. */
    virtual void timerCallback() override { repaint(); }

    /** Helper: configure a Slider as a compact horizontal slider. */
    static void makeKnob (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 14);
        s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.75f));
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    }

    /** Helper: configure a Label as a small centred parameter name. */
    static void makeLabel (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (juce::FontOptions().withHeight (13.0f)));
        l.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.55f));
    }

    /** Lay out N knob+label pairs evenly across `area`. Labels sit above knobs. */
    static void layoutKnobRow (juce::Rectangle<int> area,
                                std::initializer_list<std::pair<juce::Label*, juce::Slider*>> items)
    {
        int n = (int) items.size();
        if (n == 0) return;
        int cellW = area.getWidth() / n;

        for (auto& [lbl, sld] : items)
        {
            auto cell = area.removeFromLeft (cellW).reduced (3, 0);
            lbl->setBounds (cell.removeFromTop (16));
            sld->setBounds (cell);
        }
    }

private:
    juce::String panelTitle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModulePanel)
};
