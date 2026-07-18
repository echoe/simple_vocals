#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../PresetManager.h"

/** Horizontal bar spanning the full plugin width.
    [←]  [  Preset Name ▾  ]  [→]  [Init]  [Save]
    The name button opens a PopupMenu grouping factory and user presets.
    Save opens an AlertWindow to name and write a .svpreset file. */
class PresetBar : public juce::Component
{
public:
    explicit PresetBar (PresetManager& manager);

    void paint   (juce::Graphics&) override;
    void resized () override;

    /** Call after any preset load so the display refreshes. */
    void refresh();

private:
    PresetManager& manager;

    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::TextButton nameButton;
    juce::TextButton initButton { "Init" };
    juce::TextButton saveButton { "Save" };

    void showPresetMenu();
    void showSaveDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};
