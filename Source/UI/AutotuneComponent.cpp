#include "AutotuneComponent.h"

namespace
{
    constexpr int   kWhiteNotes[7]    = { 0, 2, 4, 5, 7, 9, 11 };
    constexpr int   kBlackNotes[5]    = { 1, 3, 6, 8, 10 };
    constexpr float kBlackOffsets[5]  = { 0.63f, 1.67f, 3.63f, 4.65f, 5.67f };
    constexpr float kBlackWidthFrac   = 0.63f;
    constexpr float kBlackHeightFrac  = 0.62f;
    const char*     kNoteNames[12]    = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };

    struct ScalePreset { const char* name; int intervals[12]; int count; };
    const ScalePreset kPresets[] = {
        { "Chromatic (All)",  {0,1,2,3,4,5,6,7,8,9,10,11}, 12 },
        { "Major",            {0,2,4,5,7,9,11},              7  },
        { "Natural Minor",    {0,2,3,5,7,8,10},              7  },
        { "Harmonic Minor",   {0,2,3,5,7,8,11},              7  },
        { "Pent. Major",      {0,2,4,7,9},                   5  },
        { "Pent. Minor",      {0,3,5,7,10},                  5  },
        { "Blues",            {0,3,5,6,7,10},                6  },
        { "Dorian",           {0,2,3,5,7,9,10},              7  },
        { "Mixolydian",       {0,2,4,5,7,9,10},              7  },
    };
    constexpr int kNumPresets = 9;
}

// ─── helpers ──────────────────────────────────────────────────────────────────

void AutotuneComponent::makeCompactSlider (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::LinearBar);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setColour (juce::Slider::trackColourId,      juce::Colour (0xff3a3a4a));
    s.setColour (juce::Slider::thumbColourId,      juce::Colour (0xff50c878));
    s.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff1e1e28));
}

void AutotuneComponent::makeLabel (juce::Label& l, const juce::String& text)
{
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centredLeft);
    l.setFont (juce::Font (juce::FontOptions().withHeight (12.5f)));
    l.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.55f));
}

float AutotuneComponent::hzToMidi (float hz)
{
    return (hz > 20.0f) ? 69.0f + 12.0f * std::log2 (hz / 440.0f) : 0.0f;
}

juce::String AutotuneComponent::noteName (int midiNote)
{
    return juce::String (kNoteNames[midiNote % 12]) + juce::String (midiNote / 12 - 1);
}

// ─── constructor ──────────────────────────────────────────────────────────────

AutotuneComponent::AutotuneComponent (juce::AudioProcessorValueTreeState& a,
                                       AutotuneModule* mod)
    : autotuneModule (mod), apvts (a)
{
    detectedHistory.fill (0.0f);
    targetHistory.fill   (0.0f);

    // Root key combo
    rootLabel.setText ("Root", juce::dontSendNotification);
    rootLabel.setFont (juce::Font (juce::FontOptions().withHeight (12.5f)));
    rootLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.55f));
    for (int i = 0; i < 12; ++i) rootBox.addItem (kNoteNames[i], i + 1);
    if (auto* p = apvts.getParameter ("auto_key"))
        rootBox.setSelectedItemIndex (juce::roundToInt (p->convertFrom0to1 (p->getValue())),
                                       juce::dontSendNotification);
    rootBox.onChange = [this] {
        if (auto* p = apvts.getParameter ("auto_key"))
        { p->beginChangeGesture();
          p->setValueNotifyingHost (p->convertTo0to1 ((float) rootBox.getSelectedItemIndex()));
          p->endChangeGesture(); }
    };
    addAndMakeVisible (rootLabel);
    addAndMakeVisible (rootBox);

    presetButton.onClick = [this] { showScalePresetMenu(); };
    addAndMakeVisible (presetButton);

    detectKeyButton.onClick = [this] { toggleKeyDetect(); };
    addAndMakeVisible (detectKeyButton);

    liveModeButton.setClickingTogglesState (false);
    studioModeButton.setClickingTogglesState (false);
    liveModeButton.onClick = [this] {
        if (auto* p = apvts.getParameter ("auto_latency_mode"))
        { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (0.0f)); p->endChangeGesture(); }
        updateLatencyModeButtons();
    };
    studioModeButton.onClick = [this] {
        if (auto* p = apvts.getParameter ("auto_latency_mode"))
        { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (1.0f)); p->endChangeGesture(); }
        updateLatencyModeButtons();
    };
    addAndMakeVisible (liveModeButton);
    addAndMakeVisible (studioModeButton);
    updateLatencyModeButtons();

    // Compact sliders
    makeCompactSlider (speedSlider);     makeLabel (speedLabel,     "Speed");
    makeCompactSlider (amountSlider);    makeLabel (amountLabel,    "Amount");
    makeCompactSlider (mixSlider);       makeLabel (mixLabel,       "Mix");
    makeCompactSlider (formantSlider);   makeLabel (formantLabel,   "Formant");
    makeCompactSlider (characterSlider); makeLabel (characterLabel, "Hard Tune");
    addAndMakeVisible (speedSlider);     addAndMakeVisible (speedLabel);
    addAndMakeVisible (amountSlider);    addAndMakeVisible (amountLabel);
    addAndMakeVisible (mixSlider);       addAndMakeVisible (mixLabel);
    addAndMakeVisible (formantSlider);   addAndMakeVisible (formantLabel);
    addAndMakeVisible (characterSlider); addAndMakeVisible (characterLabel);

    speedAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (apvts, "auto_speed",     speedSlider);
    amountAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (apvts, "auto_amount",    amountSlider);
    mixAtt       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (apvts, "auto_mix",       mixSlider);
    formantAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (apvts, "auto_formant",   formantSlider);
    characterAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (apvts, "auto_character", characterSlider);

    startTimerHz (30);
}

AutotuneComponent::~AutotuneComponent() { stopTimer(); }

// ─── timer ────────────────────────────────────────────────────────────────────

void AutotuneComponent::timerCallback()
{
    if (auto* p = apvts.getParameter ("auto_key"))
    {
        int idx = juce::roundToInt (p->convertFrom0to1 (p->getValue()));
        if (rootBox.getSelectedItemIndex() != idx)
            rootBox.setSelectedItemIndex (idx, juce::dontSendNotification);
    }
    updateLatencyModeButtons();
    if (autotuneModule != nullptr)
    {
        float hz = autotuneModule->getDetectedHz();
        detectedHistory[(size_t)(historyWriteIdx % kHistoryLen)] = hzToMidi (hz);
        targetHistory  [(size_t)(historyWriteIdx % kHistoryLen)] = hzToMidi (autotuneModule->getTargetHz());
        ++historyWriteIdx;

        if (keyDetectActive)
        {
            float conf = autotuneModule->getConfidence();
            if (hz > 20.0f && conf > 0.3f)
            {
                float midi = hzToMidi (hz);
                int   pc   = ((int) std::round (midi)) % 12;
                if (pc < 0) pc += 12;
                chromaAccum[(size_t) pc] += conf;
            }

            if (++keyDetectTicks >= kKeyDetectMaxTicks)
                finishKeyDetect();
        }
    }

    if (keyResultShowTicks > 0 && --keyResultShowTicks == 0)
        detectKeyButton.setButtonText ("Auto Key");

    repaint();
}

// ─── layout ───────────────────────────────────────────────────────────────────

void AutotuneComponent::resized()
{
    auto b = getLocalBounds().reduced (4, 4);
    auto ctrl = b.removeFromRight (kCtrlW);
    b.removeFromRight (6);

    // Row 1: Root label + combo + preset button + auto key-detect
    auto row1 = ctrl.removeFromTop (22);
    rootLabel.setBounds (row1.removeFromLeft (40));
    rootBox.setBounds   (row1.removeFromLeft (52));
    row1.removeFromLeft (4);
    detectKeyButton.setBounds (row1.removeFromRight (82));
    row1.removeFromRight (4);
    presetButton.setBounds (row1);
    ctrl.removeFromTop (4);

    // Row 1b: Live / Studio latency mode
    auto row1b = ctrl.removeFromTop (18);
    int  halfW = row1b.getWidth() / 2;
    liveModeButton.setBounds   (row1b.removeFromLeft (halfW).reduced (1, 0));
    studioModeButton.setBounds (row1b.reduced (1, 0));
    ctrl.removeFromTop (4);

    // Row 2: Piano keyboard
    int keyH = ctrl.getHeight() - 4 - 24;
    pianoRect = ctrl.removeFromTop (juce::jmax (44, keyH));
    ctrl.removeFromTop (4);

    // Row 3: 5 compact sliders
    int cellW = ctrl.getWidth() / 5;
    auto sliders = ctrl;
    struct Pair { juce::Label* l; juce::Slider* s; };
    for (auto& pr : { Pair{&speedLabel,     &speedSlider     },
                      {    &amountLabel,    &amountSlider    },
                      {    &mixLabel,       &mixSlider       },
                      {    &formantLabel,   &formantSlider   },
                      {    &characterLabel, &characterSlider } })
    {
        auto cell = sliders.removeFromLeft (cellW).reduced (2, 0);
        pr.l->setBounds (cell.removeFromTop (15));
        pr.s->setBounds (cell);
    }
}

// ─── piano keyboard ───────────────────────────────────────────────────────────

bool AutotuneComponent::getNoteEnabled (int n) const
{
    if (auto* p = apvts.getRawParameterValue ("auto_note_" + juce::String (n)))
        return p->load() > 0.5f;
    return true;
}

void AutotuneComponent::toggleNote (int n)
{
    if (auto* p = apvts.getParameter ("auto_note_" + juce::String (n)))
    { bool cur = p->getValue() > 0.5f;
      p->beginChangeGesture();
      p->setValueNotifyingHost (cur ? 0.0f : 1.0f);
      p->endChangeGesture(); }
}

int AutotuneComponent::noteAtPoint (juce::Point<float> p) const
{
    auto area = pianoRect.toFloat();
    if (! area.contains (p)) return -1;
    float x = p.x - area.getX(), y = p.y - area.getY();
    float wkw = area.getWidth() / 7.0f;
    float bkw = wkw * kBlackWidthFrac;
    float bkh = area.getHeight() * kBlackHeightFrac;
    if (y < bkh)
        for (int i = 0; i < 5; ++i)
            if (x >= kBlackOffsets[i] * wkw && x < kBlackOffsets[i] * wkw + bkw)
                return kBlackNotes[i];
    return kWhiteNotes[juce::jlimit (0, 6, (int)(x / wkw))];
}

void AutotuneComponent::drawPianoKey (juce::Graphics& g, int noteIdx, bool isBlack,
                                       juce::Rectangle<float> bounds) const
{
    bool enabled = getNoteEnabled (noteIdx);
    bool isRoot  = (noteIdx == rootBox.getSelectedItemIndex());

    juce::Colour fill;
    if (isBlack) fill = enabled ? juce::Colour (0xff3a5acc) : juce::Colour (0xff1a1a22);
    else         fill = enabled ? juce::Colour (0xff7090ff) : juce::Colours::white.withAlpha (0.92f);
    if (isRoot)  fill = fill.brighter (0.25f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, isBlack ? 2.0f : 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (bounds, isBlack ? 2.0f : 3.0f, 0.7f);

    // Label
    g.setFont (juce::Font (juce::FontOptions().withHeight (isBlack ? 6.5f : 8.0f)));
    g.setColour (isBlack
        ? (enabled ? juce::Colours::white.withAlpha (0.8f) : juce::Colours::white.withAlpha (0.18f))
        : (enabled ? juce::Colours::white.withAlpha (0.9f) : juce::Colours::black.withAlpha (0.4f)));
    g.drawText (kNoteNames[noteIdx],
                bounds.withTrimmedTop (bounds.getHeight() * (isBlack ? 0.5f : 0.7f)).toNearestInt(),
                juce::Justification::centred);

    // Root dot
    if (isRoot)
    {
        float cx = bounds.getCentreX(), cy = bounds.getBottom() - 5.0f;
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillEllipse (cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
    }
}

void AutotuneComponent::drawKeyboard (juce::Graphics& g, juce::Rectangle<int> area) const
{
    auto fa   = area.toFloat();
    float wkw = fa.getWidth() / 7.0f;
    float bkw = wkw * kBlackWidthFrac;
    float bkh = fa.getHeight() * kBlackHeightFrac;

    for (int i = 0; i < 7; ++i)
        drawPianoKey (g, kWhiteNotes[i], false,
                      { fa.getX() + (float)i * wkw + 0.5f, fa.getY(), wkw - 1.0f, fa.getHeight() });
    for (int i = 0; i < 5; ++i)
        drawPianoKey (g, kBlackNotes[i], true,
                      { fa.getX() + kBlackOffsets[i] * wkw, fa.getY(), bkw, bkh });
}

// ─── pitch display ────────────────────────────────────────────────────────────

void AutotuneComponent::drawPitchDisplay (juce::Graphics& g, juce::Rectangle<int> area) const
{
    auto fa = area.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (fa, 3.0f);

    constexpr float midiMin = 48.0f, midiMax = 76.0f;
    auto midiToY = [&](float m) {
        return fa.getBottom() - ((m - midiMin) / (midiMax - midiMin)) * fa.getHeight();
    };

    g.setFont (juce::Font (juce::FontOptions().withHeight (7.5f)));
    for (int m = (int)midiMin; m <= (int)midiMax; ++m)
    {
        float y = midiToY ((float)m);
        bool  isC = (m % 12 == 0);
        g.setColour (juce::Colours::white.withAlpha (isC ? 0.18f : 0.05f));
        g.drawLine (fa.getX(), y, fa.getRight(), y, isC ? 0.8f : 0.4f);
        if (isC) {
            g.setColour (juce::Colours::white.withAlpha (0.3f));
            g.drawText (noteName (m), (int)fa.getX(), (int)y - 7, 24, 8,
                        juce::Justification::centredLeft);
        }
    }

    if (historyWriteIdx == 0) return;

    // Target line (green)
    {
        juce::Path tp; bool started = false;
        for (int i = 0; i < kHistoryLen; ++i)
        {
            int   idx = (historyWriteIdx - kHistoryLen + i + kHistoryLen * 10) % kHistoryLen;
            float tm  = targetHistory[(size_t)idx];
            float x   = fa.getX() + (float)i / (float)kHistoryLen * fa.getWidth();
            if (tm >= midiMin && tm <= midiMax)
            { float y = midiToY (tm); if (!started) { tp.startNewSubPath(x,y); started=true; } else tp.lineTo(x,y); }
            else { started = false; }
        }
        g.setColour (juce::Colour (0xff50c878).withAlpha (0.75f));
        g.strokePath (tp, juce::PathStrokeType (2.0f));
    }

    // Detected dots (blue)
    for (int i = 0; i < kHistoryLen; ++i)
    {
        int   idx = (historyWriteIdx - kHistoryLen + i + kHistoryLen * 10) % kHistoryLen;
        float dm  = detectedHistory[(size_t)idx];
        if (dm < midiMin || dm > midiMax) continue;
        float x = fa.getX() + (float)i / (float)kHistoryLen * fa.getWidth();
        g.setColour (juce::Colour (0xff60b0ff).withAlpha (0.85f));
        g.fillEllipse (x - 1.5f, midiToY (dm) - 1.5f, 3.0f, 3.0f);
    }

    // Live readout
    if (autotuneModule != nullptr)
    {
        float det = autotuneModule->getDetectedHz(), tgt = autotuneModule->getTargetHz();
        float cor = autotuneModule->getCorrectionSt();
        auto  info = det > 20.0f
            ? noteName ((int)std::round (hzToMidi (det))) + "  \u2192  "
              + noteName ((int)std::round (hzToMidi (tgt)))
              + "   " + (cor >= 0 ? "+" : "") + juce::String (cor, 1) + " st"
            : "\u2014  no pitch";
        g.setFont (juce::Font (juce::FontOptions().withHeight (9.0f).withStyle ("Bold")));
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawText (info, area.reduced (2), juce::Justification::topRight);
    }
}

// ─── paint ────────────────────────────────────────────────────────────────────

void AutotuneComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1e1e24));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (juce::Colour (0xff50c878).withAlpha (0.35f));
    g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);

    g.setFont (juce::Font (juce::FontOptions().withHeight (11.0f).withStyle ("Bold")));
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawText ("AUTOTUNE", getLocalBounds().removeFromTop (22).reduced (8, 0),
                juce::Justification::centredLeft);

    // Pitch display (left of ctrl)
    auto displayArea = getLocalBounds().reduced (4, 4);
    displayArea.removeFromRight (kCtrlW + 6);
    displayArea.removeFromTop (22);
    drawPitchDisplay (g, displayArea);

    // Piano keyboard
    drawKeyboard (g, pianoRect);
}

// ─── mouse ────────────────────────────────────────────────────────────────────

void AutotuneComponent::mouseDown (const juce::MouseEvent& e)
{
    int note = noteAtPoint (e.position);
    if (note >= 0) toggleNote (note);
}

// ─── scale presets ────────────────────────────────────────────────────────────

void AutotuneComponent::updateLatencyModeButtons()
{
    int mode = 1;
    if (auto* p = apvts.getParameter ("auto_latency_mode"))
        mode = juce::roundToInt (p->convertFrom0to1 (p->getValue()));

    auto activeColour   = juce::Colour (0xff4466cc);
    auto inactiveColour = juce::Colour (0xff2a2a3a);
    liveModeButton.setColour   (juce::TextButton::buttonColourId, mode == 0 ? activeColour : inactiveColour);
    studioModeButton.setColour (juce::TextButton::buttonColourId, mode == 1 ? activeColour : inactiveColour);
}

void AutotuneComponent::applyScale (int rootKey, const int* intervals, int count)
{
    for (int n = 0; n < 12; ++n)
        if (auto* p = apvts.getParameter ("auto_note_" + juce::String (n)))
        { p->beginChangeGesture(); p->setValueNotifyingHost (0.0f); p->endChangeGesture(); }

    for (int i = 0; i < count; ++i)
    {
        int note = (rootKey + intervals[i]) % 12;
        if (auto* p = apvts.getParameter ("auto_note_" + juce::String (note)))
        { p->beginChangeGesture(); p->setValueNotifyingHost (1.0f); p->endChangeGesture(); }
    }
}

// ─── auto key-detect ────────────────────────────────────────────────────────

void AutotuneComponent::toggleKeyDetect()
{
    if (keyDetectActive)
    {
        finishKeyDetect();
        return;
    }

    keyDetectActive    = true;
    keyDetectTicks     = 0;
    keyResultShowTicks = 0;
    chromaAccum.fill (0.0f);
    detectKeyButton.setButtonText ("Listening...");
}

void AutotuneComponent::finishKeyDetect()
{
    keyDetectActive = false;

    float total = 0.0f;
    for (float v : chromaAccum) total += v;

    if (total < 0.001f)
    {
        detectKeyButton.setButtonText ("No signal");
        keyResultShowTicks = 45;   // ~1.5s at 30Hz, then reverts to "Auto Key"
        return;
    }

    bool isMinor = false;
    int  root    = bestKeyFromChroma (chromaAccum, isMinor);

    // Apply as Major or Natural Minor rooted at the detected key.
    // kPresets[1] = "Major", kPresets[2] = "Natural Minor" (see table above).
    const auto& preset = kPresets[isMinor ? 2 : 1];
    applyScale (root, preset.intervals, preset.count);

    rootBox.setSelectedItemIndex (root, juce::dontSendNotification);
    if (auto* p = apvts.getParameter ("auto_key"))
    { p->beginChangeGesture();
      p->setValueNotifyingHost (p->convertTo0to1 ((float) root));
      p->endChangeGesture(); }

    detectKeyButton.setButtonText (juce::String (kNoteNames[root]) + (isMinor ? " Minor" : " Major"));
    keyResultShowTicks = 60;   // ~2s at 30Hz, then reverts to "Auto Key"
}

int AutotuneComponent::bestKeyFromChroma (const std::array<float, 12>& chroma, bool& isMinor)
{
    // Krumhansl-Kessler tonal-hierarchy key profiles: how strongly each
    // scale degree is perceived to "belong" to a major/minor key, rated
    // 0-6.35 from listening studies. Correlating a pitch-class histogram
    // against rotations of these profiles is the classic Krumhansl-
    // Schmuckler key-finding algorithm.
    static constexpr float kMajorProfile[12] =
        { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
    static constexpr float kMinorProfile[12] =
        { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };

    int   bestRoot    = 0;
    bool  bestIsMinor = false;
    float bestScore   = -1.0e9f;

    for (int root = 0; root < 12; ++root)
    {
        float scoreMajor = 0.0f, scoreMinor = 0.0f;
        for (int i = 0; i < 12; ++i)
        {
            float c = chroma[(size_t) ((root + i) % 12)];
            scoreMajor += c * kMajorProfile[i];
            scoreMinor += c * kMinorProfile[i];
        }
        if (scoreMajor > bestScore) { bestScore = scoreMajor; bestRoot = root; bestIsMinor = false; }
        if (scoreMinor > bestScore) { bestScore = scoreMinor; bestRoot = root; bestIsMinor = true;  }
    }

    isMinor = bestIsMinor;
    return bestRoot;
}

void AutotuneComponent::showScalePresetMenu()
{
    int rootKey = rootBox.getSelectedItemIndex();
    juce::PopupMenu m;
    for (int i = 0; i < kNumPresets; ++i) m.addItem (i + 1, kPresets[i].name);
    m.addSeparator();
    m.addItem (100, "Enable all notes (Chromatic)");
    m.addItem (101, "Clear all notes");

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetButton),
        [this, rootKey] (int result)
        {
            if (result >= 1 && result <= kNumPresets)
                applyScale (rootKey, kPresets[result-1].intervals, kPresets[result-1].count);
            else if (result == 100 || result == 101)
                for (int n = 0; n < 12; ++n)
                    if (auto* p = apvts.getParameter ("auto_note_" + juce::String (n)))
                    { p->beginChangeGesture();
                      p->setValueNotifyingHost (result == 100 ? 1.0f : 0.0f);
                      p->endChangeGesture(); }
        });
}
