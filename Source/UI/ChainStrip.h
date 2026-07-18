#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../EffectChain.h"

/** Horizontal strip of modules running left-to-right, with drag-and-drop
    reordering and enable toggles. Everything is drawn in paint() (no child
    components) so the drag ghost can float above the other cells without
    z-order fights. Toggle state is read directly from the APVTS atomics and
    written via the parameter API. */
class ChainStrip : public juce::Component,
                   private juce::Timer
{
public:
    ChainStrip (EffectChain& chain, juce::AudioProcessorValueTreeState& apvts);
    ~ChainStrip() override;

    /** Call whenever the chain order may have changed externally. */
    void rebuild();

    void paint         (juce::Graphics&) override;
    void resized       () override {}   // bounds handled by parent
    void mouseDown     (const juce::MouseEvent&) override;
    void mouseDrag     (const juce::MouseEvent&) override;
    void mouseUp       (const juce::MouseEvent&) override;
    void mouseMove     (const juce::MouseEvent&) override;
    void mouseExit     (const juce::MouseEvent&) override;

private:
    EffectChain&                        effectChain;
    juce::AudioProcessorValueTreeState& apvts;

    struct ModInfo { juce::String name, paramId; };
    std::vector<ModInfo> modules;   // current display order

    // ── Drag state ─────────────────────────────────────────────────────────
    int  potentialDragCol = -1;  // column clicked but not yet dragging
    int  dragStartAbsX    = 0;   // absolute X where mouseDown fired
    int  dragStartColX    = 0;   // X within the column where drag started
    int  draggingIdx      = -1;  // currently dragging (-1 = none)
    int  dragCurrentX     = 0;   // current mouse X
    int  dropTargetIdx    = -1;  // where the item will land

    static constexpr int kDragThreshold = 5;
    static constexpr int kHeaderH       = 14;  // "CHAIN" label band across the top
    static constexpr int kToggleH       = 15;  // height of the toggle band along each cell's bottom
    static constexpr int kPad           = 4;

    // ── Helpers ────────────────────────────────────────────────────────────
    int                  colWidth     () const { return modules.empty() ? 0 : getWidth() / (int) modules.size(); }
    juce::Rectangle<int> colBounds    (int idx) const;
    int                  colAtX       (int x) const;
    int                  computeDropTarget (int mouseX) const;
    bool                 inToggleArea (int y) const { return y >= getHeight() - kToggleH; }

    bool getEnabled (int idx) const;
    void setEnabled (int idx, bool value);

    void drawCell (juce::Graphics&, int moduleIdx,
                   juce::Rectangle<int> bounds, bool ghost, bool dimmed) const;
    void drawToggle (juce::Graphics&, juce::Rectangle<int> area,
                     bool on, float alpha) const;

    void timerCallback() override { repaint(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChainStrip)
};
