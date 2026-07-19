#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Modules/AutotuneModule.h"

/** Full-width autotune strip.

    Left  : scrolling pitch-history graph (detected = blue dots, target = green line)
    Right : clickable piano keyboard (one octave) + Root selector + Scale preset
            button + Speed/Amount/Mix compact sliders

    The 12 individual note params (auto_note_0..11) drive both the keyboard
    display and the pitch-correction algorithm. The Scale Preset popup is
    pure UI — it writes to those params but is not itself parameterised. */
class AutotuneComponent : public juce::Component,
                          private juce::Timer
{
public:
    AutotuneComponent (juce::AudioProcessorValueTreeState& apvts, AutotuneModule* module);
    ~AutotuneComponent() override;

    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    AutotuneModule*                     autotuneModule;
    juce::AudioProcessorValueTreeState& apvts;

    // ── Pitch display ─────────────────────────────────────────────────────
    static constexpr int kHistoryLen = 300;
    std::array<float, kHistoryLen> detectedHistory {};
    std::array<float, kHistoryLen> targetHistory   {};
    int historyWriteIdx = 0;

    // ── Controls ──────────────────────────────────────────────────────────
    juce::ComboBox  rootBox;
    juce::Label     rootLabel;
    juce::TextButton presetButton    { "Scale Preset" };
    juce::TextButton detectKeyButton { "Auto Key" };

    // Auto key-detect: samples the module's already-computed detectedHz/
    // confidence at the UI timer rate (30Hz) and builds a pitch-class
    // histogram (chroma vector) while listening. On stop, correlates the
    // histogram against Krumhansl-Kessler major/minor key profiles and
    // applies the best-matching root + scale.
    bool  keyDetectActive    = false;
    int   keyDetectTicks     = 0;
    int   keyResultShowTicks = 0;
    std::array<float, 12> chromaAccum {};
    static constexpr int kKeyDetectMaxTicks = 150;  // ~5s at 30Hz

    juce::Slider speedSlider, amountSlider, mixSlider, formantSlider, characterSlider;
    juce::Label  speedLabel,  amountLabel,  mixLabel,  formantLabel,  characterLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        speedAtt, amountAtt, mixAtt, formantAtt, characterAtt;

    // ── Layout ────────────────────────────────────────────────────────────
    static constexpr int kCtrlW = 290;  // right-panel width
    juce::Rectangle<int> pianoRect;     // computed in resized, used in paint+mouseDown

    // ── Helpers ───────────────────────────────────────────────────────────
    void timerCallback() override;

    // Piano keyboard
    bool  getNoteEnabled (int note) const;
    void  toggleNote     (int note);
    int   noteAtPoint    (juce::Point<float> p) const;
    void  drawPianoKey   (juce::Graphics&, int noteIdx, bool isBlack,
                          juce::Rectangle<float> bounds) const;
    void  drawKeyboard   (juce::Graphics&, juce::Rectangle<int> area) const;

    // Pitch display
    void drawPitchDisplay (juce::Graphics&, juce::Rectangle<int> area) const;
    static float hzToMidi (float hz);
    static juce::String noteName (int midiNote);

    // Scale presets
    void showScalePresetMenu();
    void applyScale (int rootKey, const int* intervals, int count);

    // Auto key-detect
    void toggleKeyDetect();
    void finishKeyDetect();
    static int bestKeyFromChroma (const std::array<float, 12>& chroma, bool& isMinor);

    static void makeCompactSlider (juce::Slider&);
    static void makeLabel         (juce::Label&, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutotuneComponent)
};
