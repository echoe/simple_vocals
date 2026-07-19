#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../Modules/EQModule.h"

/** Visual EQ editor. Shows a labeled frequency/gain grid, a combined response
    curve, and one draggable node per active band.

    Interaction:
      Drag node        → move freq (X) and gain (Y)
      Scroll on node   → adjust Q (narrows / widens the bell)
      Double-click     → toggle dynamic mode (node turns orange)
      Right-click node → context menu: remove or toggle dynamic
      Right-click grid → context menu: add band at that frequency  */
class EQCurveComponent : public juce::Component,
                         private juce::Timer
{
public:
    explicit EQCurveComponent (juce::AudioProcessorValueTreeState& apvts,
                                juce::String idPrefix = "eq");
    ~EQCurveComponent() override;

    void paint (juce::Graphics&) override;
    void mouseDown        (const juce::MouseEvent&) override;
    void mouseDrag        (const juce::MouseEvent&) override;
    void mouseUp          (const juce::MouseEvent&) override;
    void mouseWheelMove   (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    struct BandParams
    {
        juce::RangedAudioParameter* enabled  = nullptr;
        juce::RangedAudioParameter* freq     = nullptr;
        juce::RangedAudioParameter* gain     = nullptr;
        juce::RangedAudioParameter* q        = nullptr;
        juce::RangedAudioParameter* dynamic_ = nullptr; // trailing _ avoids keyword clash
    };

    juce::AudioProcessorValueTreeState& apvts;
    juce::String idPrefix;
    std::array<BandParams, EQModule::maxBands> params;
    int draggingBand = -1;

    // Plot-area margins (pixels) inside the component bounds, reserved for axis labels
    static constexpr int marginL = 38, marginR = 10, marginT = 14, marginB = 24;
    static constexpr float minFreq = 20.0f, maxFreq = 20000.0f;
    static constexpr float minDb   = -18.0f, maxDb   = 18.0f;
    static constexpr double kPlotSR = 48000.0; // sample-rate used only for response-curve maths

    juce::Rectangle<int> plotArea() const;
    float freqToX (float f) const;
    float xToFreq (float x) const;
    float gainToY (float db) const;
    float yToGain (float y) const;

    // Accessors that safely read normalised parameter values
    bool  isEnabled  (int i) const;
    bool  isDynamic  (int i) const;
    float getBandFreq (int i) const;
    float getBandGain (int i) const;
    float getBandQ    (int i) const;

    juce::Colour nodeColour (int i) const;

    /** Magnitude response (dB) of band i at frequency freq.
        Returns 0 dB if the band is disabled or has zero gain. */
    float bandDb (int i, float freq,
                  const juce::dsp::IIR::Coefficients<float>::Ptr* cachedCoefs) const;

    int findNearestBand (juce::Point<float> pos) const;
    void addBandAt      (float freq);
    void showNodeMenu   (int band);
    void showGridMenu   (float freq);

    void timerCallback() override { repaint(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQCurveComponent)
};
