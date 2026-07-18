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

juce::Rectangle<int> ChainStrip::colBounds (int idx) const
{
    int w = colWidth();
    return { idx * w, kHeaderH, w, getHeight() - kHeaderH };
}

int ChainStrip::colAtX (int x) const
{
    int w = colWidth();
    if (w <= 0) return -1;
    int col = x / w;
    return juce::isPositiveAndBelow (col, (int) modules.size()) ? col : -1;
}

int ChainStrip::computeDropTarget (int mouseX) const
{
    int w = colWidth();
    if (w <= 0) return 0;
    int target = mouseX / w;
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

void ChainStrip::drawCell (juce::Graphics& g, int idx,
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

    // Module name — centred, above the toggle band
    auto& info = modules[(size_t) idx];
    auto nameBounds = bounds.withTrimmedBottom (kToggleH).reduced (kPad, 0);
    g.setFont (juce::Font (juce::FontOptions().withHeight (11.5f).withStyle ("Bold")));
    g.setColour (juce::Colours::white.withAlpha (alpha * (getEnabled (idx) ? 0.88f : 0.38f)));
    g.drawFittedText (info.name, nameBounds, juce::Justification::centred, 2);

    // Toggle switch — centred pill along the bottom of the cell
    auto toggleArea = juce::Rectangle<int> (bounds.getX() + bounds.getWidth() / 2 - 16,
                                             bounds.getBottom() - kToggleH,
                                             32, kToggleH);
    drawToggle (g, toggleArea.reduced (2), getEnabled (idx), alpha);
}

// ─────────────────────────────────────────────────────────── paint

void ChainStrip::paint (juce::Graphics& g)
{
    // Header
    g.setFont (juce::Font (juce::FontOptions().withHeight (9.0f).withStyle ("Bold")));
    g.setColour (juce::Colours::white.withAlpha (0.28f));
    g.drawText ("CHAIN  (drag to reorder)", 6, 0, getWidth() - 12, kHeaderH, juce::Justification::centredLeft);

    // Separator under header
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawLine (0.0f, (float) kHeaderH, (float) getWidth(), (float) kHeaderH, 0.5f);

    // Normal cells
    for (int i = 0; i < (int) modules.size(); ++i)
    {
        bool isDimmed = (i == draggingIdx);
        drawCell (g, i, colBounds (i), false, isDimmed);

        // Column right-edge separator
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        int sepX = (i + 1) * colWidth();
        g.drawLine ((float) sepX, (float) kHeaderH, (float) sepX, (float) getHeight(), 0.5f);
    }

    // Insertion indicator
    if (draggingIdx >= 0 && dropTargetIdx >= 0)
    {
        int lineX = (dropTargetIdx <= draggingIdx)
                    ? colBounds (dropTargetIdx).getX()
                    : colBounds (dropTargetIdx).getRight();

        g.setColour (juce::Colour (0xff6688ff));
        g.fillRect (lineX - 1, kHeaderH + 2, 2, getHeight() - kHeaderH - 4);

        // Small triangle markers at each end
        float tx = (float) lineX;
        float ty = (float) kHeaderH + 2.0f;
        juce::Path tri;
        tri.addTriangle (tx - 4, ty, tx + 4, ty, tx, ty + 6);
        g.fillPath (tri);
        tri.clear();
        ty = (float) getHeight() - 2.0f;
        tri.addTriangle (tx - 4, ty, tx + 4, ty, tx, ty - 6);
        g.fillPath (tri);
    }

    // Floating ghost cell (drawn last so it's always on top)
    if (draggingIdx >= 0)
    {
        int w = colWidth();
        int ghostX = juce::jlimit (0,
                                   (int) modules.size() * w - w,
                                   dragCurrentX - dragStartColX);

        // Drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (juce::Rectangle<int> (ghostX + 3, kHeaderH + 1, w, getHeight() - kHeaderH - 2).toFloat(), 3.0f);

        drawCell (g, draggingIdx, colBounds (draggingIdx).withX (ghostX), true, false);
    }
}

// ─────────────────────────────────────────────────────────── mouse

void ChainStrip::mouseDown (const juce::MouseEvent& e)
{
    int col = colAtX (e.x);
    if (col < 0 || e.y < kHeaderH) return;

    if (inToggleArea (e.y))
    {
        // Immediate toggle on click — no drag
        setEnabled (col, ! getEnabled (col));
        repaint();
        return;
    }

    // Record potential drag start
    potentialDragCol = col;
    dragStartAbsX    = e.x;
    dragStartColX    = e.x - colBounds (col).getX();
    dragCurrentX     = e.x;
    draggingIdx      = -1;
    dropTargetIdx    = -1;
}

void ChainStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (potentialDragCol < 0) return;

    // Commit to dragging once the threshold is crossed
    if (draggingIdx < 0 && std::abs (e.x - dragStartAbsX) > kDragThreshold)
    {
        draggingIdx   = potentialDragCol;
        dropTargetIdx = draggingIdx;
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    if (draggingIdx >= 0)
    {
        dragCurrentX  = e.x;
        // Ghost centre determines the target slot
        int w = colWidth();
        int ghostCentreX = dragCurrentX - dragStartColX + w / 2;
        dropTargetIdx     = computeDropTarget (ghostCentreX);
        repaint();
    }
}

void ChainStrip::mouseUp (const juce::MouseEvent&)
{
    if (draggingIdx >= 0 && dropTargetIdx >= 0 && dropTargetIdx != draggingIdx)
        effectChain.moveModule (draggingIdx, dropTargetIdx);

    draggingIdx      = -1;
    potentialDragCol = -1;
    dropTargetIdx    = -1;
    setMouseCursor (juce::MouseCursor::NormalCursor);
    rebuild();   // refresh display order from chain
}

void ChainStrip::mouseMove (const juce::MouseEvent& e)
{
    int col = colAtX (e.x);
    if (col < 0 || e.y < kHeaderH)
        setMouseCursor (juce::MouseCursor::NormalCursor);
    else if (inToggleArea (e.y))
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    else
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
}

void ChainStrip::mouseExit (const juce::MouseEvent&)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
}
