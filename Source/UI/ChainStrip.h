#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../EffectChain.h"

/** Vertical list of modules with drag-and-drop reordering and enable toggles.
    Everything is drawn in paint() (no child components) so the drag ghost can
    float above the other rows without z-order fights. Toggle state is read
    directly from the APVTS atomics and written via the parameter API. */
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
    int  potentialDragRow = -1;  // row clicked but not yet dragging
    int  dragStartAbsY    = 0;   // absolute Y where mouseDown fired
    int  dragStartRowY    = 0;   // Y within the row where drag started
    int  draggingIdx      = -1;  // currently dragging (-1 = none)
    int  dragCurrentY     = 0;   // current mouse Y
    int  dropTargetIdx    = -1;  // where the item will land

    static constexpr int kDragThreshold = 5;
    static constexpr int kHeaderH       = 18;
    static constexpr int kRowH          = 38;
    static constexpr int kToggleW       = 32;  // width of toggle area on right
    static constexpr int kPad           = 5;

    // ── Helpers ────────────────────────────────────────────────────────────
    juce::Rectangle<int> rowBounds     (int idx) const;
    int                  rowAtY        (int y) const;
    int                  computeDropTarget (int mouseY) const;
    bool                 inToggleArea  (int x) const { return x >= getWidth() - kToggleW; }

    bool getEnabled (int idx) const;
    void setEnabled (int idx, bool value);

    void drawRow (juce::Graphics&, int moduleIdx,
                  juce::Rectangle<int> bounds, bool ghost, bool dimmed) const;
    void drawToggle (juce::Graphics&, juce::Rectangle<int> area,
                     bool on, float alpha) const;

    void timerCallback() override { repaint(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChainStrip)
};
