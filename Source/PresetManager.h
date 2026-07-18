#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "EffectChain.h"
#include <vector>
#include <utility>

/** Manages factory presets and user presets for SimpleVocals.

    Factory presets are baked into the binary as C++ init functions.
    User presets are stored as .svpreset XML files in:
      ~/Documents/SimpleVocals/Presets/

    Navigation (prev / next) walks factory presets first, then user presets. */
class PresetManager
{
public:
    static constexpr const char* kExtension = ".svpreset";

    PresetManager (juce::AudioProcessorValueTreeState& apvts, EffectChain& chain);

    // ── Factory ────────────────────────────────────────────────────────────
    int getNumFactoryPresets() const { return (int) factoryPresets.size(); }
    juce::String getFactoryPresetName (int index) const;
    void loadFactoryPreset (int index);

    // ── User ───────────────────────────────────────────────────────────────
    juce::File getUserPresetsDir() const;
    juce::StringArray getUserPresetNames() const;
    void saveUserPreset (const juce::String& name);
    void loadUserPreset (const juce::String& name);
    void deleteUserPreset (const juce::String& name);

    // ── Navigation ─────────────────────────────────────────────────────────
    juce::StringArray getAllPresetNames() const;   // factory + user
    int  getCurrentPresetIndex() const;
    void navigate (int delta);                     // +1 = next, -1 = prev

    // ── State ──────────────────────────────────────────────────────────────
    juce::String getCurrentPresetName() const { return currentName; }

private:
    juce::AudioProcessorValueTreeState& apvts;
    EffectChain&                        effectChain;
    juce::String                        currentName { "Init" };

    // ── Parameter helpers ─────────────────────────────────────────────────
    /** Set a float parameter to its *actual* (denormalised) value. */
    void setParam (const juce::String& id, float value);
    /** Set a bool / choice parameter (0 or 1 for bool; index for choice). */
    void setIndex (const juce::String& id, int index);
    /** Enable only the notes in `intervals` (semitones from rootKey), disable the rest. */
    void setScale (int rootKey, const std::vector<int>& intervals);
    /** Reset every parameter to its declared default value. */
    void resetToDefaults();

    // ── Factory preset infrastructure ─────────────────────────────────────
    struct FactoryPreset
    {
        juce::String name;
        std::function<void (PresetManager&)> load;
    };
    std::vector<FactoryPreset> factoryPresets;
    void buildFactoryPresets();

    // Factory loaders (defined in PresetManager.cpp)
    void loadInit();
    void loadWarmVocal();
    void loadPopVocal();
    void loadDreamyHarmonies();
    void loadLoFi();

    // ── Data-driven factory presets ────────────────────────────────────────
    // A compact declarative format used for the extended preset library
    // (presets beyond the five hand-written loaders above). Keeps the
    // library easy to extend without a new named function per preset.
    struct PresetData
    {
        const char* name;
        std::vector<std::pair<const char*, float>> floatParams;   // id -> denormalised value
        std::vector<std::pair<const char*, int>>   indexParams;   // id -> index (bool: 0/1, choice: index)
        int rootKey = -1;                                          // -1 = no scale change
        std::vector<int> scaleIntervals;                          // semitones from rootKey
    };
    void applyPresetData (int dataIndex);
    static const std::vector<PresetData>& extendedPresetTable();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
