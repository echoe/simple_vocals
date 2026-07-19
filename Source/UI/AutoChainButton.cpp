#include "AutoChainButton.h"

AutoChainButton::AutoChainButton (SimpleVocalsAudioProcessor& proc,
                                   juce::AudioProcessorValueTreeState& a,
                                   EffectChain& chain)
    : processor (proc), apvts (a), effectChain (chain)
{
    button.setTooltip ("Listen to ~4s of vocal and build a starting chain "
                        "(EQ, compressor, de-esser, light saturation/reverb, autotune)");
    button.onClick = [this] { if (! active) start(); };
    addAndMakeVisible (button);
    startTimerHz (30);
}

AutoChainButton::~AutoChainButton() { stopTimer(); }

void AutoChainButton::resized() { button.setBounds (getLocalBounds()); }

// ─── parameter helpers ──────────────────────────────────────────────────────

void AutoChainButton::setParam (const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
    { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (value)); p->endChangeGesture(); }
}

void AutoChainButton::setBool (const juce::String& id, bool value)
{
    setParam (id, value ? 1.0f : 0.0f);
}

// ─── listening window ───────────────────────────────────────────────────────

void AutoChainButton::start()
{
    active = true;
    ticks  = 0;
    button.setButtonText ("Listening...");
    processor.startChainAnalysis();
}

void AutoChainButton::timerCallback()
{
    if (! active) return;
    if (++ticks >= kMaxTicks)
        finish();
}

void AutoChainButton::showResultThenRevert (const juce::String& text)
{
    button.setButtonText (text);
    juce::Component::SafePointer<AutoChainButton> safeThis (this);
    juce::Timer::callAfterDelay (1500, [safeThis]
    {
        if (safeThis != nullptr)
            safeThis->button.setButtonText ("Auto Chain");
    });
}

// ─── apply heuristic chain ──────────────────────────────────────────────────

void AutoChainButton::finish()
{
    active = false;
    processor.stopChainAnalysis();

    float rmsDb   = processor.getAnalysisRmsDb();
    float peakDb  = processor.getAnalysisPeakDb();
    float hfRatio = processor.getAnalysisHfRatio();
    float crest   = juce::jmax (3.0f, peakDb - rmsDb);   // dB; higher = more dynamic/uncompressed input

    if (rmsDb < -55.0f)
    {
        // Essentially silence during the listening window — bail out rather
        // than build a chain from noise-floor measurements.
        showResultThenRevert ("No signal");
        return;
    }

    // ── EQ 1: standard vocal low-cut + a touch of presence ─────────────────
    setBool  ("eq_enabled", true);
    setBool  ("eq_band0_enabled", true);
    setParam ("eq_band0_freq", 90.0f);
    setParam ("eq_band0_gain", -3.0f);
    setBool  ("eq_band2_enabled", true);
    setParam ("eq_band2_freq", 3200.0f);
    setParam ("eq_band2_gain", 1.5f);

    // ── Compressor: threshold/ratio scaled to the measured crest factor ────
    float ratio     = juce::jmap (juce::jlimit (6.0f, 24.0f, crest), 6.0f, 24.0f, 2.0f, 5.0f);
    float threshold = juce::jlimit (-40.0f, -10.0f, rmsDb + 6.0f);
    setBool  ("comp_enabled", true);
    setParam ("comp_threshold", threshold);
    setParam ("comp_ratio", ratio);
    setParam ("comp_attack", 6.0f);
    setParam ("comp_release", 90.0f);
    setParam ("comp_makeup", juce::jlimit (0.0f, 6.0f, (crest - 6.0f) * 0.3f));

    // ── De-Esser: threshold/range scaled to measured HF energy ratio ───────
    float deessRange = juce::jmap (juce::jlimit (0.0f, 0.35f, hfRatio), 0.0f, 0.35f, 3.0f, 12.0f);
    setBool  ("deess_enabled", true);
    setParam ("deess_freq", 6500.0f);
    setParam ("deess_threshold", juce::jlimit (-30.0f, -12.0f, rmsDb + 4.0f));
    setParam ("deess_range", deessRange);

    // ── Autotune: on, neutral/chromatic, light-touch (pairs with the
    //    separate "Auto Key" button if a specific key is wanted) ───────────
    setBool  ("auto_enabled", true);
    setParam ("auto_speed", 0.5f);
    setParam ("auto_amount", 0.4f);
    setParam ("auto_mix", 0.5f);
    setParam ("auto_character", 0.3f);
    for (int n = 0; n < 12; ++n)
        setBool ("auto_note_" + juce::String (n), true);   // chromatic — doesn't fight the user's key choice

    // ── Saturation: light "glue" ─────────────────────────────────────────
    setBool  ("sat_enabled", true);
    setParam ("sat_mode", 0.0f);   // Tape
    setParam ("sat_drive", 1.5f);
    setParam ("sat_mix", 0.12f);

    // ── Reverb: light space ─────────────────────────────────────────────
    setBool  ("verb_enabled", true);
    setParam ("verb_size", 0.35f);
    setParam ("verb_damping", 0.5f);
    setParam ("verb_predelay", 15.0f);
    setParam ("verb_mix", 0.14f);

    // ── Left off: stylistic choices, not corrective ones ────────────────
    setBool ("harm_enabled", false);
    setBool ("dly_enabled",  false);
    setBool ("eq2_enabled",  false);

    // ── Reset processing order to the sensible default flow ────────────
    effectChain.resetOrder();

    showResultThenRevert ("Chain Set!");
}
