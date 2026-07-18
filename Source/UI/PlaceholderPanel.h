#pragma once
#include "ModulePanel.h"

/** Temporary placeholder for modules whose UI isn't built yet. */
class PlaceholderPanel : public ModulePanel
{
public:
    PlaceholderPanel (const juce::String& title, juce::Colour accent)
        : ModulePanel (title, accent) {}

    void paint (juce::Graphics& g) override
    {
        ModulePanel::paint (g);
        g.setColour (juce::Colours::white.withAlpha (0.2f));
        g.setFont (juce::Font (juce::FontOptions().withHeight (11.0f)));
        g.drawText ("Coming soon", contentArea(), juce::Justification::centred);
    }

    void resized() override {}
};
