#include "EQCurveComponent.h"

namespace
{
    constexpr float nodeR   = 7.0f;   // node circle radius
    constexpr float hitR    = 14.0f;  // hit-test radius

    const float  gridFreqs[]  = { 30, 60, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 };
    const char*  freqLabels[] = { "30", "60", "125", "250", "500", "1k", "2k", "4k", "8k", "16k" };
    const float  gridGains[]  = { -18, -12, -6, 0, 6, 12, 18 };
}

// ------------------------------------------------------------------ lifecycle

EQCurveComponent::EQCurveComponent (juce::AudioProcessorValueTreeState& a, juce::String prefix)
    : apvts (a), idPrefix (std::move (prefix))
{
    for (int i = 0; i < EQModule::maxBands; ++i)
    {
        auto& p = params[(size_t) i];
        p.enabled  = apvts.getParameter (EQModule::bandParamID (idPrefix, i, "enabled"));
        p.freq     = apvts.getParameter (EQModule::bandParamID (idPrefix, i, "freq"));
        p.gain     = apvts.getParameter (EQModule::bandParamID (idPrefix, i, "gain"));
        p.q        = apvts.getParameter (EQModule::bandParamID (idPrefix, i, "q"));
        p.dynamic_ = apvts.getParameter (EQModule::bandParamID (idPrefix, i, "dynamic"));
    }
    setInterceptsMouseClicks (true, false);
    startTimerHz (30);
}

EQCurveComponent::~EQCurveComponent() { stopTimer(); }

// ---------------------------------------------------------------- coordinate helpers

juce::Rectangle<int> EQCurveComponent::plotArea() const
{
    return { marginL, marginT,
             getWidth()  - marginL - marginR,
             getHeight() - marginT - marginB };
}

float EQCurveComponent::freqToX (float f) const
{
    auto pa = plotArea();
    auto t  = std::log (f / minFreq) / std::log (maxFreq / minFreq);
    return (float) pa.getX() + t * (float) pa.getWidth();
}

float EQCurveComponent::xToFreq (float x) const
{
    auto pa = plotArea();
    auto t  = juce::jlimit (0.0f, 1.0f, (x - (float) pa.getX()) / (float) pa.getWidth());
    return minFreq * std::pow (maxFreq / minFreq, t);
}

float EQCurveComponent::gainToY (float db) const
{
    auto pa = plotArea();
    auto t  = (db - minDb) / (maxDb - minDb);           // 0 = bottom, 1 = top
    return (float) pa.getBottom() - t * (float) pa.getHeight();
}

float EQCurveComponent::yToGain (float y) const
{
    auto pa = plotArea();
    auto t  = juce::jlimit (0.0f, 1.0f,
                             1.0f - (y - (float) pa.getY()) / (float) pa.getHeight());
    return minDb + t * (maxDb - minDb);
}

// ---------------------------------------------------------------- parameter accessors

bool  EQCurveComponent::isEnabled  (int i) const { auto& p = params[(size_t)i]; return p.enabled  && p.enabled->getValue()  > 0.5f; }
bool  EQCurveComponent::isDynamic  (int i) const { auto& p = params[(size_t)i]; return p.dynamic_ && p.dynamic_->getValue() > 0.5f; }

float EQCurveComponent::getBandFreq (int i) const
{
    auto& p = params[(size_t) i];
    return p.freq ? p.freq->convertFrom0to1 (p.freq->getValue()) : 1000.0f;
}
float EQCurveComponent::getBandGain (int i) const
{
    auto& p = params[(size_t) i];
    return p.gain ? p.gain->convertFrom0to1 (p.gain->getValue()) : 0.0f;
}
float EQCurveComponent::getBandQ (int i) const
{
    auto& p = params[(size_t) i];
    return p.q ? p.q->convertFrom0to1 (p.q->getValue()) : 1.0f;
}

juce::Colour EQCurveComponent::nodeColour (int i) const
{
    return isDynamic (i) ? juce::Colours::orangered : juce::Colour (0xff4fa3e0);
}

// ---------------------------------------------------------------- response maths

float EQCurveComponent::bandDb (int i, float freq,
                                 const juce::dsp::IIR::Coefficients<float>::Ptr* cache) const
{
    if (! isEnabled (i)) return 0.0f;
    if (cache[i] == nullptr) return 0.0f;
    auto mag = (float) cache[i]->getMagnitudeForFrequency ((double) freq, kPlotSR);
    return juce::Decibels::gainToDecibels (mag, -100.0f);
}

// ---------------------------------------------------------------- paint

void EQCurveComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto pa     = plotArea();

    // Background
    g.setColour (juce::Colour (0xff1a1a1f));
    g.fillRoundedRectangle (bounds, 6.0f);

    // ── Frequency gridlines + labels ──────────────────────────────────────
    g.setFont (10.0f);
    for (int fi = 0; fi < 10; ++fi)
    {
        auto x = freqToX (gridFreqs[fi]);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawLine (x, (float) pa.getY(), x, (float) pa.getBottom(), 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawText (freqLabels[fi],
                    juce::Rectangle<int> ((int) x - 16, pa.getBottom() + 4, 32, 14),
                    juce::Justification::centred);
    }

    // ── Gain gridlines + labels ───────────────────────────────────────────
    for (auto gdb : gridGains)
    {
        auto y      = gainToY (gdb);
        bool isZero = (std::abs (gdb) < 0.5f);
        g.setColour (juce::Colours::white.withAlpha (isZero ? 0.22f : 0.07f));
        g.drawLine ((float) pa.getX(), y, (float) pa.getRight(), y, isZero ? 1.0f : 0.5f);

        auto label = (gdb > 0.0f ? "+" : "") + juce::String ((int) gdb);
        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawText (label,
                    juce::Rectangle<int> (0, (int) y - 7, marginL - 4, 14),
                    juce::Justification::centredRight);
    }

    // ── Pre-compute one coefficient set per band (avoids per-pixel allocation) ──
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, EQModule::maxBands> coefCache;
    for (int i = 0; i < EQModule::maxBands; ++i)
    {
        if (! isEnabled (i) || std::abs (getBandGain (i)) < 0.01f) continue;
        coefCache[(size_t) i] = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            kPlotSR, getBandFreq (i), getBandQ (i),
            juce::Decibels::decibelsToGain (getBandGain (i)));
    }

    // ── Per-band curves (visual Q indicator) ─────────────────────────────
    for (int i = 0; i < EQModule::maxBands; ++i)
    {
        if (! isEnabled (i) || coefCache[(size_t) i] == nullptr) continue;

        juce::Path bp;
        bool started = false;
        for (int x = pa.getX(); x <= pa.getRight(); x += 2)
        {
            auto y = juce::jlimit ((float) pa.getY(), (float) pa.getBottom(),
                                   gainToY (bandDb (i, xToFreq ((float) x), coefCache.data())));
            if (! started) { bp.startNewSubPath ((float) x, y); started = true; }
            else             bp.lineTo ((float) x, y);
        }
        g.setColour (nodeColour (i).withAlpha (0.22f));
        g.strokePath (bp, juce::PathStrokeType (1.0f));
    }

    // ── Combined response curve ───────────────────────────────────────────
    {
        juce::Path cp;
        bool started = false;
        for (int x = pa.getX(); x <= pa.getRight(); x += 2)
        {
            float total = 0.0f;
            auto  freq  = xToFreq ((float) x);
            for (int i = 0; i < EQModule::maxBands; ++i)
                total += bandDb (i, freq, coefCache.data());

            auto y = juce::jlimit ((float) pa.getY(), (float) pa.getBottom(), gainToY (total));
            if (! started) { cp.startNewSubPath ((float) x, y); started = true; }
            else             cp.lineTo ((float) x, y);
        }
        g.setColour (juce::Colour (0xffb080f5));
        g.strokePath (cp, juce::PathStrokeType (2.0f));
    }

    // ── Nodes ─────────────────────────────────────────────────────────────
    g.setFont (8.5f);
    for (int i = 0; i < EQModule::maxBands; ++i)
    {
        if (! isEnabled (i)) continue;

        juce::Point<float> np { freqToX (getBandFreq (i)), gainToY (getBandGain (i)) };
        auto col = nodeColour (i);

        g.setColour (col);
        g.fillEllipse (np.x - nodeR, np.y - nodeR, nodeR * 2, nodeR * 2);
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.drawEllipse (np.x - nodeR, np.y - nodeR, nodeR * 2, nodeR * 2, 1.4f);
        g.setColour (juce::Colours::white);
        g.drawText (juce::String (i + 1),
                    juce::Rectangle<float> (np.x - nodeR, np.y - nodeR, nodeR * 2, nodeR * 2),
                    juce::Justification::centred);
    }

    // ── Drag tooltip ──────────────────────────────────────────────────────
    if (draggingBand >= 0)
    {
        auto  freq   = getBandFreq (draggingBand);
        auto  gain   = getBandGain (draggingBand);
        auto  q      = getBandQ    (draggingBand);
        juce::Point<float> np { freqToX (freq), gainToY (gain) };

        auto freqStr = freq < 1000.0f ? juce::String ((int) freq) + " Hz"
                                       : juce::String (freq / 1000.0f, 2) + " kHz";
        auto gainStr = (gain >= 0.0f ? "+" : "") + juce::String (gain, 1) + " dB";
        auto qStr    = "Q " + juce::String (q, 2);
        auto tip     = freqStr + "   " + gainStr + "   " + qStr;

        g.setFont (11.0f);
        constexpr int tw = 210, th = 20;
        auto tx = juce::jlimit ((float) pa.getX(), (float) (pa.getRight() - tw), np.x - tw / 2.0f);
        auto ty = np.y < (float) (pa.getY() + 40)
                  ? np.y + nodeR + 5.0f
                  : np.y - nodeR - th - 4.0f;
        g.setColour (juce::Colours::black.withAlpha (0.75f));
        g.fillRoundedRectangle (tx, ty, tw, th, 3.0f);
        g.setColour (juce::Colours::white);
        g.drawText (tip, (int) tx, (int) ty, tw, th, juce::Justification::centred);
    }

    // ── Band count hint ───────────────────────────────────────────────────
    int active = 0;
    for (int i = 0; i < EQModule::maxBands; ++i)
        if (isEnabled (i)) ++active;

    g.setFont (9.5f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawText (juce::String (active) + "/" + juce::String (EQModule::maxBands)
                    + " bands   scroll = Q   dbl-click = dynamic   right-click = add/remove",
                pa.getX(), 2, pa.getWidth(), marginT - 2,
                juce::Justification::centredRight);
}

// ---------------------------------------------------------------- mouse

int EQCurveComponent::findNearestBand (juce::Point<float> pos) const
{
    int   nearest = -1;
    float nearestDist = hitR;
    for (int i = 0; i < EQModule::maxBands; ++i)
    {
        if (! isEnabled (i)) continue;
        juce::Point<float> np { freqToX (getBandFreq (i)), gainToY (getBandGain (i)) };
        auto d = np.getDistanceFrom (pos);
        if (d < nearestDist) { nearestDist = d; nearest = i; }
    }
    return nearest;
}

void EQCurveComponent::mouseDown (const juce::MouseEvent& e)
{
    auto band = findNearestBand (e.position);

    if (e.mods.isRightButtonDown())
    {
        if (band >= 0) showNodeMenu (band);
        else           showGridMenu (xToFreq (e.position.x));
        return;
    }

    if (band >= 0)
    {
        draggingBand = band;
        auto& p = params[(size_t) band];
        if (p.freq) p.freq->beginChangeGesture();
        if (p.gain) p.gain->beginChangeGesture();
    }
}

void EQCurveComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingBand < 0) return;
    auto& p  = params[(size_t) draggingBand];
    auto  pa = plotArea();
    auto  cx = juce::jlimit ((float) pa.getX(),  (float) pa.getRight(),  e.position.x);
    auto  cy = juce::jlimit ((float) pa.getY(),  (float) pa.getBottom(), e.position.y);

    if (p.freq) p.freq->setValueNotifyingHost (p.freq->convertTo0to1 (xToFreq (cx)));
    if (p.gain) p.gain->setValueNotifyingHost (p.gain->convertTo0to1 (yToGain (cy)));
    repaint();
}

void EQCurveComponent::mouseUp (const juce::MouseEvent&)
{
    if (draggingBand < 0) return;
    auto& p = params[(size_t) draggingBand];
    if (p.freq) p.freq->endChangeGesture();
    if (p.gain) p.gain->endChangeGesture();
    draggingBand = -1;
    repaint();
}

void EQCurveComponent::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    auto band = findNearestBand (e.position);
    if (band < 0) return;

    auto& p   = params[(size_t) band];
    if (! p.q) return;

    auto newQ = juce::jlimit (0.2f, 8.0f,
                               p.q->convertFrom0to1 (p.q->getValue()) + w.deltaY * 3.0f);
    p.q->beginChangeGesture();
    p.q->setValueNotifyingHost (p.q->convertTo0to1 (newQ));
    p.q->endChangeGesture();
    repaint();
}

void EQCurveComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto band = findNearestBand (e.position);
    if (band < 0) return;

    auto& p = params[(size_t) band];
    if (! p.dynamic_) return;

    auto was = p.dynamic_->getValue() > 0.5f;
    p.dynamic_->beginChangeGesture();
    p.dynamic_->setValueNotifyingHost (was ? 0.0f : 1.0f);
    p.dynamic_->endChangeGesture();
    repaint();
}

// ---------------------------------------------------------------- context menus

void EQCurveComponent::addBandAt (float freq)
{
    for (int i = 0; i < EQModule::maxBands; ++i)
    {
        if (isEnabled (i)) continue; // skip already-active bands

        auto& p = params[(size_t) i];
        if (p.freq)    { p.freq->beginChangeGesture();
                         p.freq->setValueNotifyingHost (p.freq->convertTo0to1 (freq));
                         p.freq->endChangeGesture(); }
        if (p.gain)    { p.gain->beginChangeGesture();
                         p.gain->setValueNotifyingHost (p.gain->convertTo0to1 (0.0f));
                         p.gain->endChangeGesture(); }
        if (p.enabled) { p.enabled->beginChangeGesture();
                         p.enabled->setValueNotifyingHost (1.0f);
                         p.enabled->endChangeGesture(); }
        break;
    }
}

void EQCurveComponent::showNodeMenu (int band)
{
    juce::PopupMenu m;
    m.addItem (1, "Remove band " + juce::String (band + 1));
    m.addSeparator();
    m.addItem (2, isDynamic (band) ? "Switch to static" : "Switch to dynamic");

    m.showMenuAsync (juce::PopupMenu::Options(), [this, band] (int result)
    {
        if (result == 1)
        {
            auto& p = params[(size_t) band];
            if (p.enabled) { p.enabled->beginChangeGesture();
                             p.enabled->setValueNotifyingHost (0.0f);
                             p.enabled->endChangeGesture(); }
        }
        else if (result == 2)
        {
            auto& p = params[(size_t) band];
            if (p.dynamic_) { auto was = p.dynamic_->getValue() > 0.5f;
                              p.dynamic_->beginChangeGesture();
                              p.dynamic_->setValueNotifyingHost (was ? 0.0f : 1.0f);
                              p.dynamic_->endChangeGesture(); }
        }
    });
}

void EQCurveComponent::showGridMenu (float freq)
{
    int active = 0;
    for (int i = 0; i < EQModule::maxBands; ++i)
        if (isEnabled (i)) ++active;

    if (active >= EQModule::maxBands) return;

    auto freqStr = freq < 1000.0f ? juce::String ((int) freq) + " Hz"
                                   : juce::String (freq / 1000.0f, 1) + " kHz";
    juce::PopupMenu m;
    m.addItem (1, "Add band at " + freqStr);

    m.showMenuAsync (juce::PopupMenu::Options(), [this, freq] (int result)
    {
        if (result == 1)
            addBandAt (freq);
    });
}
