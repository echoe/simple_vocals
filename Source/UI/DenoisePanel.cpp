#include "DenoisePanel.h"

DenoisePanel::DenoisePanel (juce::AudioProcessorValueTreeState& a, NoiseReductionModule* module)
    : HalfModulePanel ("DENOISE", juce::Colour (0xff8090ff)),
      apvts (a), denoiseModule (module)
{
    learnButton.onClick = [this]
    {
        // The module defaults OFF (it does nothing without a profile, and
        // adds latency/CPU while active), but that meant clicking "Learn"
        // silently did nothing if the module hadn't separately been enabled
        // via the chain strip — process() (and therefore the learning
        // logic) is never called for a bypassed module. Learn should just
        // work, so force the module on here.
        if (auto* p = apvts.getParameter ("denoise_enabled"))
        {
            if (p->getValue() < 0.5f)
            { p->beginChangeGesture(); p->setValueNotifyingHost (1.0f); p->endChangeGesture(); }
        }
        if (denoiseModule != nullptr)
            denoiseModule->startLearning();
    };
    addAndMakeVisible (learnButton);

    staticModeButton.setClickingTogglesState (false);
    adaptiveModeButton.setClickingTogglesState (false);
    staticModeButton.onClick = [this]
    {
        if (auto* p = apvts.getParameter ("denoise_mode"))
        { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (0.0f)); p->endChangeGesture(); }
        updateModeButtons();
    };
    adaptiveModeButton.onClick = [this]
    {
        if (auto* p = apvts.getParameter ("denoise_mode"))
        { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (1.0f)); p->endChangeGesture(); }
        updateModeButtons();
    };
    addAndMakeVisible (staticModeButton);
    addAndMakeVisible (adaptiveModeButton);
    updateModeButtons();

    makeKnob (reductionSlider);   makeLabel (reductionLabel,   "Reduction");
    makeKnob (sensitivitySlider); makeLabel (sensitivityLabel, "Sensitivity");
    addAndMakeVisible (reductionSlider);
    addAndMakeVisible (sensitivitySlider);
    addAndMakeVisible (reductionLabel);
    addAndMakeVisible (sensitivityLabel);

    reductionAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "denoise_reduction", reductionSlider);
    sensitivityAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "denoise_sensitivity", sensitivitySlider);
}

void DenoisePanel::updateModeButtons()
{
    int mode = 0;
    if (auto* p = apvts.getParameter ("denoise_mode"))
        mode = juce::roundToInt (p->convertFrom0to1 (p->getValue()));

    auto activeColour   = juce::Colour (0xff4466cc);
    auto inactiveColour = juce::Colour (0xff2a2a3a);
    staticModeButton.setColour   (juce::TextButton::buttonColourId, mode == 0 ? activeColour : inactiveColour);
    adaptiveModeButton.setColour (juce::TextButton::buttonColourId, mode == 1 ? activeColour : inactiveColour);
}

void DenoisePanel::timerCallback()
{
    HalfModulePanel::timerCallback();   // keeps the base class's 30Hz repaint

    if (denoiseModule == nullptr) return;

    updateModeButtons();

    juce::String text = "Learn Noise";
    if (denoiseModule->isLearning())      text = "Listening...";
    else if (denoiseModule->hasProfile()) text = "Re-Learn";

    if (learnButton.getButtonText() != text)
        learnButton.setButtonText (text);
}

void DenoisePanel::resized()
{
    auto row = contentArea();   // use the full height, not a fixed 38px slice

    learnButton.setBounds       (row.removeFromLeft (78).withSizeKeepingCentre (78, 22));
    row.removeFromLeft (3);
    staticModeButton.setBounds  (row.removeFromLeft (46).withSizeKeepingCentre (46, 22));
    row.removeFromLeft (2);
    adaptiveModeButton.setBounds(row.removeFromLeft (56).withSizeKeepingCentre (56, 22));
    row.removeFromLeft (6);

    layoutKnobRow (row, { { &reductionLabel,   &reductionSlider   },
                           { &sensitivityLabel, &sensitivitySlider } });
}
