#include "ChainStrip.h"

ChainStrip::ChainStrip (EffectChain& c, juce::AudioProcessorValueTreeState& a)
    : effectChain (c), apvts (a)
{
    setInterceptsMouseClicks (true, false);
    rebuild();
    startTimerHz (30);
}

ChainStrip::~ChainStrip() { stopTimer(); }

// ─────────────────────────────────────────────────────────── data

void ChainStrip::rebuild()
{
    modules.clear();
    for (auto* m : effectChain.getModulesInOrder())
        modules.push_back ({ m->getDisplayName(), m->getId() + "_enabled" });
    repaint();
}

// ─────────────────────────────────────────────────────────── helpers

juce::Rectangle<int> ChainStrip::rowBounds (int idx) const
{
    return { 0, kHeaderH + idx * kRowH, getWidth(), kRowH };
}

int ChainStrip::rowAtY (int y) const
{
    if (y < kHeaderH) return -1;
    int row = (y - kHeaderH) / kRowH;
    return juce::isPositiveAndBelow (row, (int) modules.size()) ? row : -1;
}

int ChainStrip::computeDropTarget (int mouseY) const
{
    // The drop slot is determined by which half of a row the ghost centre is in
    int rel = mouseY - kHeaderH;
    int target = rel / kRowH;
    return juce::jlimit (0, (int) modules.size() - 1, target);
}

bool ChainStrip::getEnabled (int idx) const
{
    if (auto* p = apvts.getRawParameterValue (modules[(size_t) idx].paramId))
        return p->load() > 0.5f;
    return true;
}

void ChainStrip::setEnabled (int idx, bool value)
{
    if (auto* p = apvts.getParameter (modules[(size_t) idx].paramId))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (value ? 1.0f : 0.0f);
        p->endChangeGesture();
    }
}

// ─────────────────────────────────────────────────────────── drawing

void ChainStrip::drawToggle (juce::Graphics& g, juce::Rectangle<int> area,
                              bool on, float alpha) const
{
    auto pill = area.reduced (2).toFloat();
    float r   = pill.getHeight() * 0.5f;

    // Track
    g.setColour ((on ? juce::Colour (0xff4466cc) : juce::Colour (0xff2a2a3a)).withAlpha (alpha));
    g.fillRoundedRectangle (pill, r);
    g.setColour (juce::Colours::white.withAlpha (alpha * 0.15f));
    g.drawRoundedRectangle (pill, r, 0.8f);

    // Knob
    float knobR  = r - 2.0f;
    float knobX  = on ? pill.getRight() - knobR - 2.0f : pill.getX() + knobR + 2.0f;
    float knobY  = pill.getCentreY();
    g.setColour (juce::Colours::white.withAlpha (alpha * 0.92f));
    g.fillEllipse (knobX - knobR, knobY - knobR, knobR * 2.0f, knobR * 2.0f);
}

void ChainStrip::drawRow (juce::Graphics& g, int idx,
                           juce::Rectangle<int> bounds,
                           bool ghost, bool dimmed) const
{
    float alpha = dimmed ? 0.22f : 1.0f;

    // Background
    if (ghost)
    {
        g.setColour (juce::Colour (0xff2e2e48).withAlpha (0.97f));
        g.fillRoundedRectangle (bounds.reduced (1).toFloat(), 3.0f);
        g.setColour (juce::Colour (0xff6688ff).withAlpha (0.55f));
        g.drawRoundedRectangle (bounds.reduced (1).toFloat(), 3.0f, 1.2f);
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha ((idx % 2 == 0 ? 0.025f : 0.0f) * alpha));
        g.fillRect (bounds);
    }

    // Drag-handle dots (three dots on left edge)
    if (! ghost)
    {
        float cx  = (float) kPad + 3.0f;
        float cy0 = (float) bounds.getCentreY() - 4.0f;
        g.setColour (juce::Colours::white.withAlpha (0.2f * alpha));
        for (int d = 0; d < 3; ++d)
            g.fillEllipse (cx - 1.5f, cy0 + (float) d * 4.0f - 1.5f, 3.0f, 3.0f);
    }

    // Module name
    auto& info = modules[(size_t) idx];
    g.setFont (juce::Font (juce::FontOptions().withHeight (12.0f).withStyle ("Bold")));
    g.setColour (juce::Colours::white.withAlpha (alpha * (getEnabled (idx) ? 0.88f : 0.38f)));
    auto textBounds = bounds.reduced (kPad + 10, 0).withTrimmedRight (kToggleW);
    g.drawText (info.name, textBounds, juce::Justification::centredLeft);

    // Toggle switch
    auto toggleArea = juce::Rectangle<int> (bounds.getRight() - kToggleW,
                                             bounds.getY(),
                                             kToggleW,
                                             bounds.getHeight());
    drawToggle (g, toggleArea.reduced (4, 10), getEnabled (idx), alpha);
}

// ─────────────────────────────────────────────────────────── paint

void ChainStrip::paint (juce::Graphics& g)
{
    // Header
    g.setFont (juce::Font (juce::FontOptions().withHeight (9.5f).withStyle ("Bold")));
    g.setColour (juce::Colours::white.withAlpha (0.28f));
    g.drawText ("CHAIN", 0, 0, getWidth(), kHeaderH, juce::Justification::centred);

    // Separator under header
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawLine (4.0f, (float) kHeaderH, (float) getWidth() - 4.0f, (float) kHeaderH, 0.5f);

    // Normal rows
    for (int i = 0; i < (int) modules.size(); ++i)
    {
        bool isDimmed = (i == draggingIdx);
        drawRow (g, i, rowBounds (i), false, isDimmed);

        // Row bottom separator
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        int sepY = kHeaderH + (i + 1) * kRowH;
        g.drawLine (4.0f, (float) sepY, (float) getWidth() - 4.0f, (float) sepY, 0.5f);
    }

    // Insertion indicator
    if (draggingIdx >= 0 && dropTargetIdx >= 0)
    {
        int lineY = (dropTargetIdx <= draggingIdx)
                    ? rowBounds (dropTargetIdx).getY()
                    : rowBounds (dropTargetIdx).getBottom();

        g.setColour (juce::Colour (0xff6688ff));
        g.fillRect (4, lineY - 1, getWidth() - 8, 2);

        // Small triangle markers at each end
        float tx  = 3.0f;
        float ty  = (float) lineY;
        juce::Path tri;
        tri.addTriangle (tx, ty - 4, tx, ty + 4, tx + 6, ty);
        g.fillPath (tri);
        tri.clear();
        tx = (float) (getWidth() - 3);
        tri.addTriangle (tx, ty - 4, tx, ty + 4, tx - 6, ty);
        g.fillPath (tri);
    }

    // Floating ghost row (drawn last so it's always on top)
    if (draggingIdx >= 0)
    {
        int ghostY = juce::jlimit (kHeaderH,
                                   kHeaderH + (int) modules.size() * kRowH - kRowH,
                                   dragCurrentY - dragStartRowY);

        // Drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (juce::Rectangle<int> (1, ghostY + 3, getWidth() - 2, kRowH).toFloat(), 3.0f);

        drawRow (g, draggingIdx, rowBounds (draggingIdx).withY (ghostY), true, false);
    }
}

// ─────────────────────────────────────────────────────────── mouse

void ChainStrip::mouseDown (const juce::MouseEvent& e)
{
    int row = rowAtY (e.y);
    if (row < 0) return;

    if (inToggleArea (e.x))
    {
        // Immediate toggle on click — no drag
        setEnabled (row, ! getEnabled (row));
        repaint();
        return;
    }

    // Record potential drag start
    potentialDragRow = row;
    dragStartAbsY    = e.y;
    dragStartRowY    = e.y - rowBounds (row).getY();
    dragCurrentY     = e.y;
    draggingIdx      = -1;
    dropTargetIdx    = -1;
}

void ChainStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (potentialDragRow < 0) return;

    // Commit to dragging once the threshold is crossed
    if (draggingIdx < 0 && std::abs (e.y - dragStartAbsY) > kDragThreshold)
    {
        draggingIdx   = potentialDragRow;
        dropTargetIdx = draggingIdx;
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    if (draggingIdx >= 0)
    {
        dragCurrentY  = e.y;
        // Ghost centre determines the target slot
        int ghostCentreY = dragCurrentY - dragStartRowY + kRowH / 2;
        dropTargetIdx    = computeDropTarget (ghostCentreY);
        repaint();
    }
}

void ChainStrip::mouseUp (const juce::MouseEvent&)
{
    if (draggingIdx >= 0 && dropTargetIdx >= 0 && dropTargetIdx != draggingIdx)
        effectChain.moveModule (draggingIdx, dropTargetIdx);

    draggingIdx      = -1;
    potentialDragRow = -1;
    dropTargetIdx    = -1;
    setMouseCursor (juce::MouseCursor::NormalCursor);
    rebuild();   // refresh display order from chain
}

void ChainStrip::mouseMove (const juce::MouseEvent& e)
{
    int row = rowAtY (e.y);
    if (row < 0)
        setMouseCursor (juce::MouseCursor::NormalCursor);
    else if (inToggleArea (e.x))
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    else
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void ChainStrip::mouseExit (const juce::MouseEvent&)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
}
