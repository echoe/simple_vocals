#include "PresetManager.h"

// ──────────────────────────────────────────────────────── construction

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& a, EffectChain& c)
    : apvts (a), effectChain (c)
{
    buildFactoryPresets();
}

// ──────────────────────────────────────────────────────── parameter helpers

void PresetManager::setParam (const juce::String& id, float value)
{
    if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (id)))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void PresetManager::setIndex (const juce::String& id, int index)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
}
void PresetManager::setScale (int rootKey, const std::vector<int>& intervals)
{
    // Disable all 12 notes first
    for (int n = 0; n < 12; ++n)
        setIndex ("auto_note_" + juce::String (n), 0);
    // Enable notes in the scale (relative to rootKey)
    for (int interval : intervals)
        setIndex ("auto_note_" + juce::String ((rootKey + interval) % 12), 1);
}


void PresetManager::resetToDefaults()
{
    for (auto* param : apvts.processor.getParameters())
        param->setValueNotifyingHost (param->getDefaultValue());
    effectChain.resetOrder();
}

// ──────────────────────────────────────────────────────── factory presets

void PresetManager::buildFactoryPresets()
{
    factoryPresets.push_back ({ "Init",               [](PresetManager& m){ m.loadInit();              } });
    factoryPresets.push_back ({ "Warm Vocal",          [](PresetManager& m){ m.loadWarmVocal();         } });
    factoryPresets.push_back ({ "Pop Vocal",           [](PresetManager& m){ m.loadPopVocal();          } });
    factoryPresets.push_back ({ "Dreamy Harmonies",   [](PresetManager& m){ m.loadDreamyHarmonies();   } });
    factoryPresets.push_back ({ "Lo-Fi",               [](PresetManager& m){ m.loadLoFi();              } });

    // Remaining presets are declarative (see extendedPresetTable) so the
    // library can grow without a hand-written loader per preset.
    const auto& table = extendedPresetTable();
    for (int i = 0; i < (int) table.size(); ++i)
        factoryPresets.push_back ({ table[(size_t) i].name,
                                     [i] (PresetManager& m) { m.applyPresetData (i); } });
}

// ── Data-driven preset application ───────────────────────────────────────

void PresetManager::applyPresetData (int dataIndex)
{
    const auto& table = extendedPresetTable();
    if (! juce::isPositiveAndBelow (dataIndex, (int) table.size())) return;
    const auto& p = table[(size_t) dataIndex];

    resetToDefaults();

    for (auto& fp : p.floatParams) setParam (fp.first, fp.second);
    for (auto& ip : p.indexParams) setIndex (ip.first, ip.second);
    if (p.rootKey >= 0) setScale (p.rootKey, p.scaleIntervals);
}

const std::vector<PresetManager::PresetData>& PresetManager::extendedPresetTable()
{
    static const std::vector<PresetData> table =
    {
        // ── Radio / broadcast / spoken word ─────────────────────────────
        { "Radio Ready",
          { {"eq_band0_freq",100.0f},{"eq_band0_gain",-3.0f},{"eq_band2_freq",3000.0f},{"eq_band2_gain",3.0f},
            {"comp_threshold",-18.0f},{"comp_ratio",4.0f},{"comp_attack",5.0f},{"comp_release",80.0f},{"comp_makeup",4.0f},
            {"deess_freq",6500.0f},{"deess_threshold",-20.0f},{"deess_range",9.0f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"dly_enabled",0} }, -1, {} },

        { "Podcast Voice",
          { {"eq_band0_freq",90.0f},{"eq_band0_gain",-4.0f},{"eq_band1_freq",250.0f},{"eq_band1_gain",-2.0f},
            {"comp_threshold",-24.0f},{"comp_ratio",3.0f},{"comp_attack",8.0f},{"comp_release",100.0f},{"comp_makeup",3.0f},
            {"deess_freq",6000.0f},{"deess_threshold",-22.0f},{"deess_range",7.0f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"dly_enabled",0},{"verb_enabled",0} }, -1, {} },

        { "Broadcast Voiceover",
          { {"eq_band0_freq",100.0f},{"eq_band0_gain",-6.0f},{"eq_band2_freq",4000.0f},{"eq_band2_gain",2.0f},
            {"comp_threshold",-20.0f},{"comp_ratio",5.0f},{"comp_attack",3.0f},{"comp_release",60.0f},{"comp_makeup",5.0f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"dly_enabled",0},{"verb_enabled",0} }, -1, {} },

        { "Cinematic Trailer VO",
          { {"eq_band0_freq",90.0f},{"eq_band0_gain",-2.0f},{"eq_band2_freq",2500.0f},{"eq_band2_gain",3.0f},
            {"comp_threshold",-16.0f},{"comp_ratio",4.5f},{"comp_attack",4.0f},{"comp_release",70.0f},{"comp_makeup",5.0f},
            {"verb_size",0.45f},{"verb_damping",0.4f},{"verb_predelay",25.0f},{"verb_mix",0.2f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"dly_enabled",0} }, -1, {} },

        // ── Pop / mainstream ─────────────────────────────────────────────
        { "Bright Pop",
          { {"eq_band2_freq",3000.0f},{"eq_band2_gain",3.0f},{"eq_band3_freq",9000.0f},{"eq_band3_gain",2.5f},
            {"auto_speed",0.08f},{"auto_amount",0.9f},
            {"comp_threshold",-19.0f},{"comp_ratio",3.5f},{"comp_attack",6.0f},{"comp_release",70.0f},{"comp_makeup",3.0f},
            {"verb_size",0.35f},{"verb_mix",0.15f},{"dly_time",180.0f},{"dly_feedback",0.2f},{"dly_mix",0.1f} },
          { {"harm_enabled",0} }, 0, {0,2,4,5,7,9,11} },

        { "Modern Pop Polish",
          { {"eq_band1_freq",300.0f},{"eq_band1_gain",1.5f},{"eq_band3_freq",10000.0f},{"eq_band3_gain",2.0f},
            {"auto_speed",0.04f},{"auto_amount",1.0f},
            {"comp_threshold",-20.0f},{"comp_ratio",4.0f},{"comp_attack",5.0f},{"comp_release",65.0f},{"comp_makeup",3.5f},
            {"deess_threshold",-19.0f},{"verb_size",0.4f},{"verb_mix",0.15f} },
          { {"harm_enabled",0} }, 0, {0,2,4,5,7,9,11} },

        { "Stadium Anthem",
          { {"eq_band2_gain",2.5f},{"comp_threshold",-16.0f},{"comp_ratio",4.5f},{"comp_makeup",5.0f},
            {"harm_interval1",4.0f},{"harm_interval2",7.0f},{"harm_mix",0.3f},
            {"verb_size",0.65f},{"verb_damping",0.35f},{"verb_width",1.0f},{"verb_mix",0.28f},
            {"dly_time",350.0f},{"dly_feedback",0.3f},{"dly_mix",0.15f} },
          {}, -1, {} },

        { "Anthemic Chorus",
          { {"harm_interval1",7.0f},{"harm_interval2",12.0f},{"harm_mix",0.4f},{"harm_width",0.8f},
            {"verb_size",0.6f},{"verb_mix",0.25f},{"comp_threshold",-18.0f},{"comp_ratio",3.5f},{"comp_makeup",3.0f} },
          {}, -1, {} },

        { "EDM Vocal Chop",
          { {"auto_speed",0.02f},{"auto_amount",1.0f},{"comp_threshold",-16.0f},{"comp_ratio",6.0f},
            {"comp_attack",1.5f},{"comp_release",40.0f},{"comp_makeup",4.0f},
            {"dly_time",125.0f},{"dly_feedback",0.35f},{"dly_mix",0.2f},{"verb_mix",0.12f} },
          { {"harm_enabled",0} }, 0, {0,2,4,5,7,9,11} },

        { "Nightcore Bright",
          { {"eq_band2_gain",3.0f},{"eq_band3_freq",11000.0f},{"eq_band3_gain",3.5f},
            {"auto_speed",0.03f},{"auto_amount",1.0f},{"auto_formant",2.0f},
            {"comp_threshold",-18.0f},{"comp_ratio",4.0f},{"comp_makeup",3.5f} },
          { {"harm_enabled",0} }, 0, {0,2,4,5,7,9,11} },

        // ── R&B / Hip-Hop / Trap ─────────────────────────────────────────
        { "R&B Silk",
          { {"eq_band0_freq",120.0f},{"eq_band0_gain",1.5f},{"eq_band2_freq",2800.0f},{"eq_band2_gain",1.5f},
            {"comp_threshold",-20.0f},{"comp_ratio",3.0f},{"comp_attack",10.0f},{"comp_release",110.0f},{"comp_makeup",2.5f},
            {"sat_mode",1.0f},{"sat_drive",2.0f},{"sat_mix",0.15f},
            {"harm_interval1",3.0f},{"harm_interval2",7.0f},{"harm_mix",0.2f},
            {"verb_size",0.4f},{"verb_mix",0.16f} },
          {}, -1, {} },

        { "Silky R&B Runs",
          { {"auto_speed",0.1f},{"auto_amount",0.6f},{"comp_threshold",-22.0f},{"comp_ratio",2.5f},
            {"comp_attack",12.0f},{"comp_release",130.0f},{"harm_interval1",3.0f},{"harm_interval2",7.0f},{"harm_mix",0.25f},
            {"verb_size",0.42f},{"verb_mix",0.18f} },
          {}, 0, {0,2,3,5,7,8,10} },

        { "Trap Vocal",
          { {"eq_band0_freq",100.0f},{"eq_band0_gain",-2.0f},{"eq_band2_freq",3200.0f},{"eq_band2_gain",2.5f},
            {"auto_speed",0.015f},{"auto_amount",1.0f},
            {"comp_threshold",-17.0f},{"comp_ratio",5.0f},{"comp_attack",3.0f},{"comp_release",50.0f},{"comp_makeup",4.5f},
            {"dly_time",250.0f},{"dly_feedback",0.25f},{"dly_mix",0.14f} },
          { {"harm_enabled",0} }, 0, {0,1,3,5,6,8,10} },

        { "Dark Trap Autotune",
          { {"eq_band0_freq",100.0f},{"eq_band0_gain",-3.0f},
            {"auto_speed",0.01f},{"auto_amount",1.0f},{"auto_formant",-1.5f},
            {"comp_threshold",-16.0f},{"comp_ratio",6.0f},{"comp_makeup",5.0f},
            {"sat_mode",2.0f},{"sat_drive",3.0f},{"sat_mix",0.2f},
            {"dly_time",280.0f},{"dly_feedback",0.3f},{"dly_mix",0.16f} },
          { {"harm_enabled",0} }, 0, {0,1,3,5,6,8,10} },

        { "Heavy Autotune (T-Style)",
          { {"auto_speed",0.0f},{"auto_amount",1.0f},{"auto_mix",1.0f},
            {"comp_threshold",-18.0f},{"comp_ratio",4.0f},{"comp_makeup",3.0f} },
          { {"harm_enabled",0} }, 0, {0,2,4,5,7,9,11} },

        { "Punchy Rap Vocal",
          { {"eq_band2_freq",3000.0f},{"eq_band2_gain",3.0f},
            {"comp_threshold",-15.0f},{"comp_ratio",5.5f},{"comp_attack",2.0f},{"comp_release",45.0f},{"comp_makeup",5.0f},
            {"deess_threshold",-18.0f},{"deess_range",10.0f} },
          { {"auto_enabled",0},{"harm_enabled",0} }, -1, {} },

        // ── Rock / Metal ─────────────────────────────────────────────────
        { "Arena Rock Vocal",
          { {"eq_band2_freq",2500.0f},{"eq_band2_gain",3.0f},{"eq_band3_gain",1.5f},
            {"comp_threshold",-16.0f},{"comp_ratio",4.0f},{"comp_attack",4.0f},{"comp_release",70.0f},{"comp_makeup",4.0f},
            {"sat_mode",1.0f},{"sat_drive",3.5f},{"sat_mix",0.25f},
            {"verb_size",0.45f},{"verb_mix",0.18f},{"dly_time",220.0f},{"dly_feedback",0.25f},{"dly_mix",0.12f} },
          { {"auto_enabled",0},{"harm_enabled",0} }, -1, {} },

        { "Aggressive Rock Shout",
          { {"eq_band2_gain",4.0f},{"comp_threshold",-14.0f},{"comp_ratio",5.0f},{"comp_attack",2.0f},
            {"comp_release",50.0f},{"comp_makeup",4.5f},{"sat_mode",2.0f},{"sat_drive",6.0f},{"sat_mix",0.35f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"verb_enabled",0} }, -1, {} },

        { "Metal Scream",
          { {"eq_band0_freq",130.0f},{"eq_band0_gain",2.0f},{"eq_band2_gain",4.0f},
            {"comp_threshold",-12.0f},{"comp_ratio",6.0f},{"comp_attack",1.0f},{"comp_release",35.0f},{"comp_makeup",5.0f},
            {"sat_mode",2.0f},{"sat_drive",9.0f},{"sat_mix",0.5f},{"sat_tone",2.0f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"verb_enabled",0} }, -1, {} },

        // ── Vintage / lo-fi / character ──────────────────────────────────
        { "Vintage Ribbon",
          { {"eq_band0_freq",90.0f},{"eq_band0_gain",1.5f},{"eq_band3_freq",7000.0f},{"eq_band3_gain",-2.5f},
            {"sat_mode",0.0f},{"sat_drive",3.0f},{"sat_mix",0.3f},{"sat_tone",-1.5f},
            {"comp_threshold",-20.0f},{"comp_ratio",2.5f},{"comp_attack",15.0f},{"comp_release",150.0f} },
          { {"auto_enabled",0},{"harm_enabled",0} }, -1, {} },

        { "Old Vinyl",
          { {"eq_band0_freq",150.0f},{"eq_band0_gain",-3.0f},{"eq_band3_freq",6000.0f},{"eq_band3_gain",-6.0f},
            {"sat_mode",0.0f},{"sat_drive",4.0f},{"sat_mix",0.45f},{"sat_tone",-3.0f},
            {"verb_size",0.3f},{"verb_damping",0.8f},{"verb_mix",0.12f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"dly_enabled",0} }, -1, {} },

        { "Lo-Fi Bedroom Pop",
          { {"eq_band0_freq",130.0f},{"eq_band0_gain",-4.0f},{"eq_band3_freq",7500.0f},{"eq_band3_gain",-4.0f},
            {"sat_mode",2.0f},{"sat_drive",4.0f},{"sat_mix",0.4f},
            {"comp_threshold",-18.0f},{"comp_ratio",3.0f},{"comp_makeup",2.0f},
            {"verb_size",0.3f},{"verb_mix",0.12f},{"dly_time",160.0f},{"dly_feedback",0.35f},{"dly_mix",0.2f} },
          { {"auto_enabled",0},{"harm_enabled",0} }, -1, {} },

        { "Telephone FX",
          { {"eq_band0_freq",400.0f},{"eq_band0_gain",-18.0f},{"eq_band3_freq",3400.0f},{"eq_band3_gain",-18.0f},
            {"sat_mode",2.0f},{"sat_drive",5.0f},{"sat_mix",0.6f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"verb_enabled",0},{"dly_enabled",0} }, -1, {} },

        { "Robot Voice",
          { {"auto_speed",0.0f},{"auto_amount",1.0f},{"auto_character",1.0f},{"auto_mix",1.0f} },
          { {"harm_enabled",0},{"verb_enabled",0} }, 0, {0,2,4,6,8,10} },

        // ── Ambient / cinematic / atmosphere ─────────────────────────────
        { "Ethereal Whisper",
          { {"eq_band0_freq",150.0f},{"eq_band0_gain",-4.0f},{"eq_band3_gain",2.0f},
            {"comp_threshold",-28.0f},{"comp_ratio",2.0f},{"comp_makeup",2.0f},
            {"harm_interval1",7.0f},{"harm_interval2",12.0f},{"harm_mix",0.35f},{"harm_width",0.9f},
            {"verb_size",0.85f},{"verb_damping",0.2f},{"verb_mix",0.4f} },
          { {"auto_enabled",0} }, -1, {} },

        { "Ambient Pad Vocal",
          { {"harm_interval1",5.0f},{"harm_interval2",12.0f},{"harm_mix",0.5f},{"harm_grain",120.0f},{"harm_width",1.0f},
            {"verb_size",0.9f},{"verb_damping",0.15f},{"verb_mix",0.5f},
            {"dly_time",600.0f},{"dly_feedback",0.45f},{"dly_mix",0.25f} },
          {}, -1, {} },

        { "Cathedral Reverb",
          { {"verb_size",0.95f},{"verb_damping",0.15f},{"verb_predelay",35.0f},{"verb_mix",0.42f},{"verb_width",1.0f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"dly_enabled",0} }, -1, {} },

        { "Gentle Lullaby",
          { {"eq_band0_gain",1.0f},{"comp_threshold",-24.0f},{"comp_ratio",2.0f},{"comp_attack",15.0f},{"comp_release",180.0f},
            {"verb_size",0.5f},{"verb_damping",0.5f},{"verb_mix",0.2f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"dly_enabled",0} }, -1, {} },

        { "Ballad Emotional",
          { {"eq_band1_gain",1.0f},{"eq_band2_gain",1.5f},
            {"comp_threshold",-22.0f},{"comp_ratio",2.8f},{"comp_attack",10.0f},{"comp_release",120.0f},{"comp_makeup",2.5f},
            {"verb_size",0.55f},{"verb_damping",0.4f},{"verb_mix",0.22f} },
          { {"auto_enabled",0},{"harm_enabled",0} }, -1, {} },

        // ── Live / band / room ───────────────────────────────────────────
        { "Live Concert",
          { {"comp_threshold",-18.0f},{"comp_ratio",3.5f},{"comp_attack",6.0f},{"comp_release",80.0f},{"comp_makeup",3.0f},
            {"verb_size",0.55f},{"verb_damping",0.35f},{"verb_predelay",20.0f},{"verb_mix",0.22f},
            {"dly_time",300.0f},{"dly_feedback",0.25f},{"dly_mix",0.12f} },
          { {"auto_enabled",0},{"harm_enabled",0} }, -1, {} },

        { "Live Room Ambience",
          { {"eq_band0_gain",-1.5f},{"verb_size",0.5f},{"verb_damping",0.45f},{"verb_predelay",12.0f},{"verb_mix",0.2f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"dly_enabled",0} }, -1, {} },

        { "Acoustic Intimate",
          { {"eq_band0_freq",100.0f},{"eq_band0_gain",-1.0f},{"eq_band2_freq",2200.0f},{"eq_band2_gain",1.0f},
            {"comp_threshold",-24.0f},{"comp_ratio",2.2f},{"comp_attack",12.0f},{"comp_release",140.0f},
            {"verb_size",0.35f},{"verb_mix",0.14f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"dly_enabled",0} }, -1, {} },

        // ── Choir / harmony-forward ───────────────────────────────────────
        { "Church Choir",
          { {"harm_interval1",4.0f},{"harm_interval2",7.0f},{"harm_mix",0.5f},{"harm_humanise",0.5f},{"harm_width",1.0f},
            {"verb_size",0.8f},{"verb_damping",0.25f},{"verb_predelay",25.0f},{"verb_mix",0.35f} },
          {}, 0, {0,2,4,5,7,9,11} },

        { "Gospel Choir",
          { {"harm_interval1",3.0f},{"harm_interval2",7.0f},{"harm_mix",0.45f},{"harm_humanise",0.4f},
            {"comp_threshold",-18.0f},{"comp_ratio",3.5f},{"comp_makeup",3.5f},
            {"verb_size",0.65f},{"verb_mix",0.28f} },
          {}, 0, {0,2,3,5,7,8,10} },

        { "Doubling Thicken",
          { {"harm_interval1",0.0f},{"harm_interval2",0.0f},{"harm_mix",0.4f},{"harm_humanise",0.6f},{"harm_grain",100.0f},
            {"harm_width",0.7f} },
          { {"auto_enabled",0} }, -1, {} },

        { "Wide Stereo Chorus",
          { {"harm_interval1",0.0f},{"harm_interval2",12.0f},{"harm_mix",0.35f},{"harm_width",1.0f},{"harm_humanise",0.4f} },
          { {"auto_enabled",0} }, -1, {} },

        // ── Genre character ───────────────────────────────────────────────
        { "Jazz Club",
          { {"eq_band0_freq",120.0f},{"eq_band0_gain",1.0f},{"eq_band2_freq",2500.0f},{"eq_band2_gain",1.0f},
            {"comp_threshold",-22.0f},{"comp_ratio",2.5f},{"comp_attack",14.0f},{"comp_release",160.0f},
            {"sat_mode",0.0f},{"sat_drive",1.5f},{"sat_mix",0.15f},
            {"verb_size",0.45f},{"verb_mix",0.18f} },
          { {"auto_enabled",0},{"harm_enabled",0} }, -1, {} },

        { "Country Twang",
          { {"eq_band2_freq",2800.0f},{"eq_band2_gain",2.5f},{"eq_band3_freq",8500.0f},{"eq_band3_gain",1.5f},
            {"comp_threshold",-20.0f},{"comp_ratio",3.0f},{"comp_makeup",2.5f},
            {"verb_size",0.35f},{"verb_mix",0.15f},{"dly_time",180.0f},{"dly_feedback",0.2f},{"dly_mix",0.1f} },
          { {"auto_enabled",0},{"harm_enabled",0} }, -1, {} },

        { "Retro Disco",
          { {"eq_band2_gain",2.0f},{"comp_threshold",-18.0f},{"comp_ratio",3.5f},{"comp_attack",6.0f},
            {"comp_release",70.0f},{"comp_makeup",3.0f},{"harm_interval1",7.0f},{"harm_mix",0.2f},
            {"verb_size",0.4f},{"verb_mix",0.16f},{"dly_time",220.0f},{"dly_feedback",0.3f},{"dly_mix",0.15f} },
          {}, -1, {} },

        { "Synthwave Vocoder",
          { {"auto_speed",0.0f},{"auto_amount",1.0f},{"auto_character",0.9f},{"auto_formant",-2.0f},
            {"dly_time",375.0f},{"dly_feedback",0.4f},{"dly_mix",0.2f},{"verb_size",0.5f},{"verb_mix",0.2f} },
          { {"harm_enabled",0} }, 0, {0,2,3,5,7,8,10} },

        // ── Special / utility ─────────────────────────────────────────────
        { "Whisper Intimate",
          { {"eq_band0_freq",150.0f},{"eq_band0_gain",-6.0f},{"eq_band3_gain",3.0f},
            {"comp_threshold",-30.0f},{"comp_ratio",2.0f},{"comp_attack",15.0f},{"comp_release",150.0f},{"comp_makeup",4.0f},
            {"deess_threshold",-24.0f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"dly_enabled",0} }, -1, {} },

        { "ASMR Soft",
          { {"eq_band0_freq",120.0f},{"eq_band0_gain",-3.0f},{"eq_band3_freq",12000.0f},{"eq_band3_gain",2.5f},
            {"comp_threshold",-32.0f},{"comp_ratio",1.8f},{"comp_attack",18.0f},{"comp_release",180.0f},
            {"deess_threshold",-26.0f},{"deess_range",14.0f},
            {"verb_size",0.25f},{"verb_mix",0.1f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"dly_enabled",0} }, -1, {} },

        { "Subtle Polish",
          { {"eq_band2_gain",1.0f},{"comp_threshold",-22.0f},{"comp_ratio",2.2f},{"comp_attack",10.0f},
            {"comp_release",110.0f},{"comp_makeup",1.5f},{"deess_threshold",-22.0f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"dly_enabled",0} }, -1, {} },

        { "Deep Radio DJ",
          { {"eq_band0_freq",90.0f},{"eq_band0_gain",3.0f},{"eq_band3_freq",8000.0f},{"eq_band3_gain",-1.0f},
            {"comp_threshold",-18.0f},{"comp_ratio",4.0f},{"comp_attack",6.0f},{"comp_release",80.0f},{"comp_makeup",4.0f} },
          { {"auto_enabled",0},{"harm_enabled",0},{"dly_enabled",0},{"verb_enabled",0} }, -1, {} },

        { "Slap Rockabilly",
          { {"eq_band2_freq",2600.0f},{"eq_band2_gain",2.0f},
            {"comp_threshold",-18.0f},{"comp_ratio",3.5f},{"comp_makeup",3.0f},
            {"sat_mode",0.0f},{"sat_drive",2.5f},{"sat_mix",0.2f},
            {"dly_time",110.0f},{"dly_feedback",0.15f},{"dly_mix",0.22f} },
          { {"auto_enabled",0},{"harm_enabled",0} }, -1, {} },
    };
    return table;
}

// ── Init ──────────────────────────────────────────────────────────────────

void PresetManager::loadInit()
{
    resetToDefaults();
    // Bypass the more transformative modules by default so the plugin is transparent
    setIndex ("auto_enabled",  0);
    setIndex ("harm_enabled",  0);
    setIndex ("dly_enabled",   0);
}

// ── Warm Vocal ────────────────────────────────────────────────────────────

void PresetManager::loadWarmVocal()
{
    resetToDefaults();

    // Light EQ: small low-mid warmth boost
    setParam ("eq_band1_freq", 300.0f);
    setParam ("eq_band1_gain", 2.0f);

    // De-esser: gentle
    setParam ("deess_freq",      6500.0f);
    setParam ("deess_threshold", -22.0f);
    setParam ("deess_range",     8.0f);

    // Compressor: gentle glue
    setParam ("comp_threshold", -22.0f);
    setParam ("comp_ratio",     2.5f);
    setParam ("comp_attack",    12.0f);
    setParam ("comp_release",   120.0f);
    setParam ("comp_makeup",    2.0f);

    // Tape saturation
    setIndex ("sat_mode",  0);   // Tape
    setParam ("sat_drive", 2.5f);
    setParam ("sat_mix",   0.25f);

    // Small room reverb
    setParam ("verb_size",    0.3f);
    setParam ("verb_damping", 0.65f);
    setParam ("verb_mix",     0.15f);

    // Bypass transformative modules
    setIndex ("auto_enabled",  0);
    setIndex ("harm_enabled",  0);
    setIndex ("dly_enabled",   0);
}

// ── Pop Vocal ─────────────────────────────────────────────────────────────

void PresetManager::loadPopVocal()
{
    resetToDefaults();

    // EQ: slight high boost for presence
    setParam ("eq_band2_freq", 2500.0f);
    setParam ("eq_band2_gain", 2.5f);
    setParam ("eq_band3_freq", 8000.0f);
    setParam ("eq_band3_gain", 2.0f);

    // Autotune: tight
    setParam ("auto_speed",  0.05f);
    setParam ("auto_amount", 1.0f);
    setScale (0, {0,2,4,5,7,9,11});   // C Major

    // De-esser
    setParam ("deess_freq",      7000.0f);
    setParam ("deess_threshold", -20.0f);
    setParam ("deess_range",     10.0f);

    // Compressor: punchy
    setParam ("comp_threshold", -20.0f);
    setParam ("comp_ratio",     4.0f);
    setParam ("comp_attack",    5.0f);
    setParam ("comp_release",   60.0f);
    setParam ("comp_makeup",    3.0f);

    // Plate reverb
    setParam ("verb_size",     0.5f);
    setParam ("verb_damping",  0.4f);
    setParam ("verb_mix",      0.18f);
    setParam ("verb_predelay", 18.0f);

    // Eighth-note slapback
    setParam ("dly_time",     125.0f);
    setParam ("dly_feedback", 0.2f);
    setParam ("dly_mix",      0.12f);

    // Bypass harmonizer
    setIndex ("harm_enabled", 0);
}

// ── Dreamy Harmonies ──────────────────────────────────────────────────────

void PresetManager::loadDreamyHarmonies()
{
    resetToDefaults();

    // Gentle compression
    setParam ("comp_threshold", -18.0f);
    setParam ("comp_ratio",     2.5f);
    setParam ("comp_makeup",    1.5f);

    // Tape warmth
    setIndex ("sat_mode",  0);
    setParam ("sat_drive", 2.0f);
    setParam ("sat_mix",   0.2f);

    // Harmonizer: minor third + fifth
    setParam ("harm_interval1", 3.0f);   // minor 3rd
    setParam ("harm_interval2", 7.0f);   // perfect 5th
    setParam ("harm_mix",       0.45f);
    setParam ("harm_grain",     90.0f);

    // Lush long reverb
    setParam ("verb_size",    0.75f);
    setParam ("verb_damping", 0.25f);
    setParam ("verb_width",   1.0f);
    setParam ("verb_mix",     0.3f);

    // Ping-pong delay
    setParam ("dly_time",     480.0f);
    setParam ("dly_feedback", 0.4f);
    setParam ("dly_mix",      0.18f);
    setIndex ("dly_pingpong", 1);

    // Bypass autotune + de-esser
    setIndex ("auto_enabled",  0);
    setIndex ("deess_enabled", 0);
}

// ── Lo-Fi ─────────────────────────────────────────────────────────────────

void PresetManager::loadLoFi()
{
    resetToDefaults();

    // EQ: roll off highs and lows
    setParam ("eq_band0_freq", 120.0f);
    setParam ("eq_band0_gain", -6.0f);
    setParam ("eq_band3_freq", 8000.0f);
    setParam ("eq_band3_gain", -5.0f);

    // Compression: heavy, fast
    setParam ("comp_threshold", -15.0f);
    setParam ("comp_ratio",     6.0f);
    setParam ("comp_attack",    2.0f);
    setParam ("comp_release",   40.0f);
    setParam ("comp_makeup",    4.0f);

    // Hard clip saturation
    setIndex ("sat_mode",  2);   // Clip
    setParam ("sat_drive", 5.0f);
    setParam ("sat_mix",   0.55f);
    setParam ("sat_tone",  -2.0f);

    // Dark room reverb
    setParam ("verb_size",    0.28f);
    setParam ("verb_damping", 0.8f);
    setParam ("verb_mix",     0.1f);

    // Short slapback delay with dark repeats
    setParam ("dly_time",     160.0f);
    setParam ("dly_feedback", 0.45f);
    setParam ("dly_mix",      0.25f);
    setParam ("dly_tone",     1800.0f);

    // Bypass autotune + harmonizer
    setIndex ("auto_enabled", 0);
    setIndex ("harm_enabled", 0);
}

// ──────────────────────────────────────────────────────── factory accessors

juce::String PresetManager::getFactoryPresetName (int index) const
{
    if (juce::isPositiveAndBelow (index, (int) factoryPresets.size()))
        return factoryPresets[(size_t) index].name;
    return {};
}

void PresetManager::loadFactoryPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) factoryPresets.size())) return;
    factoryPresets[(size_t) index].load (*this);
    currentName = factoryPresets[(size_t) index].name;
}

// ──────────────────────────────────────────────────────── user presets

juce::File PresetManager::getUserPresetsDir() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("SimpleVocals")
                   .getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}

juce::StringArray PresetManager::getUserPresetNames() const
{
    juce::StringArray names;
    for (auto& f : getUserPresetsDir().findChildFiles (juce::File::findFiles, false,
                                                        "*" + juce::String (kExtension)))
        names.add (f.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

void PresetManager::saveUserPreset (const juce::String& name)
{
    if (name.isEmpty()) return;

    auto state = apvts.copyState();
    effectChain.appendOrderToState (state);

    juce::ValueTree root ("SimpleVocalsPreset");
    root.setProperty ("name", name, nullptr);
    root.appendChild (state, nullptr);

    auto file = getUserPresetsDir().getChildFile (name + kExtension);
    if (auto xml = root.createXml())
        xml->writeTo (file);

    currentName = name;
}

void PresetManager::loadUserPreset (const juce::String& name)
{
    auto file = getUserPresetsDir().getChildFile (name + kExtension);
    if (! file.existsAsFile()) return;

    if (auto xml = juce::XmlDocument::parse (file))
    {
        auto root  = juce::ValueTree::fromXml (*xml);
        auto state = root.getChildWithName ("PARAMETERS");
        if (state.isValid())
        {
            effectChain.restoreOrderFromState (state);
            apvts.replaceState (state);
            currentName = name;
        }
    }
}

void PresetManager::deleteUserPreset (const juce::String& name)
{
    getUserPresetsDir().getChildFile (name + kExtension).deleteFile();
    if (currentName == name) currentName = "Init";
}

// ──────────────────────────────────────────────────────── navigation

juce::StringArray PresetManager::getAllPresetNames() const
{
    juce::StringArray all;
    for (auto& fp : factoryPresets) all.add (fp.name);
    all.addArray (getUserPresetNames());
    return all;
}

int PresetManager::getCurrentPresetIndex() const
{
    auto all = getAllPresetNames();
    return all.indexOf (currentName);
}

void PresetManager::navigate (int delta)
{
    auto all   = getAllPresetNames();
    int  idx   = getCurrentPresetIndex();
    int  newIdx = (idx + delta + all.size()) % all.size();
    auto name   = all[newIdx];

    int numFactory = (int) factoryPresets.size();
    if (newIdx < numFactory)
        loadFactoryPreset (newIdx);
    else
        loadUserPreset (name);
}
