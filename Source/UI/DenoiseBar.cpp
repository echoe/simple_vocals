#include "DenoiseBar.h"

DenoiseBar::DenoiseBar (juce::AudioProcessorValueTreeState& a, NoiseReductionModule* module)
    : apvts (a), denoiseModule (module)
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
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (1.0f);
                p->endChangeGesture();
            }
        }

        if (denoiseModule != nullptr)
            denoiseModule->startLearning();
    };
    addAndMakeVisible (learnButton);

    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setFont (juce::Font (juce::FontOptions().withHeight (10.0f)));
    addAndMakeVisible (statusLabel);

    auto makeKnob = [] (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 13);
        s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.75f));
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    };
    auto makeLbl = [] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (juce::FontOptions().withHeight (12.0f)));
        l.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.55f));
    };

    makeKnob (reductionSlider);   makeLbl (reductionLabel,   "Reduction");
    makeKnob (sensitivitySlider); makeLbl (sensitivityLabel, "Sensitivity");

    addAndMakeVisible (reductionSlider);
    addAndMakeVisible (sensitivitySlider);
    addAndMakeVisible (reductionLabel);
    addAndMakeVisible (sensitivityLabel);

    reductionAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "denoise_reduction", reductionSlider);
    sensitivityAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, "denoise_sensitivity", sensitivitySlider);

    startTimerHz (15);
}

DenoiseBar::~DenoiseBar() { stopTimer(); }

void DenoiseBar::timerCallback()
{
    if (denoiseModule == nullptr) return;

    juce::String text   = "Learn Noise Profile";
    juce::String status = "No profile learned";
    auto colour = juce::Colours::white.withAlpha (0.4f);

    if (denoiseModule->isLearning())
    {
        text   = "Listening...";
        status = "Listening for noise...";
        colour = juce::Colour (0xffe0b040);
    }
    else if (denoiseModule->hasProfile())
    {
        text   = "Re-Learn Profile";
        status = "Profile learned — ready";
        colour = juce::Colour (0xff60c080);
    }

    if (learnButton.getButtonText() != text)
        learnButton.setButtonText (text);
    statusLabel.setText (status, juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, colour);
}

void DenoiseBar::resized()
{
    auto b = getLocalBounds().reduced (8, 4);
    b.removeFromTop (16);   // room for the "DENOISE" title drawn in paint()

    auto left = b.removeFromLeft (170);
    learnButton.setBounds (left.removeFromTop (24));
    left.removeFromTop (2);
    statusLabel.setBounds (left);

    b.removeFromLeft (10);

    int cellW = b.getWidth() / 2;
    auto reductionCell = b.removeFromLeft (cellW).reduced (6, 0);
    reductionLabel.setBounds (reductionCell.removeFromTop (14));
    reductionSlider.setBounds (reductionCell);

    auto sensitivityCell = b.reduced (6, 0);
    sensitivityLabel.setBounds (sensitivityCell.removeFromTop (14));
    sensitivitySlider.setBounds (sensitivityCell);
}

void DenoiseBar::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff1e1e24));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (juce::Colour (0xff8090ff).withAlpha (0.4f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    g.setFont (juce::Font (juce::FontOptions().withHeight (11.0f).withStyle ("Bold")));
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawText ("DENOISE", 12, 2, getWidth() - 24, 16, juce::Justification::centredLeft);
}
