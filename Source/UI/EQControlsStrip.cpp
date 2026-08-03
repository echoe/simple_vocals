#include "EQControlsStrip.h"

void EQControlsStrip::setupSlider (juce::Slider& s, juce::Colour thumbColour)
{
    s.setSliderStyle (juce::Slider::LinearHorizontal);
    s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 38, 16);
    s.setColour (juce::Slider::trackColourId,      juce::Colour (0xff3a3a4a));
    s.setColour (juce::Slider::thumbColourId,      thumbColour);
    s.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff222230));
    s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.8f));
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void EQControlsStrip::setupLabel (juce::Label& l, const juce::String& text)
{
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centredLeft);
    l.setFont (juce::Font (juce::FontOptions().withHeight (10.5f)));
    l.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.55f));
}

EQControlsStrip::EQControlsStrip (juce::AudioProcessorValueTreeState& apvts, juce::String idPrefix)
{
    for (int i = 0; i < EQModule::maxBands; ++i)
    {
        auto& band = bands[(size_t) i];

        // Enable toggle
        band.enableToggle.setButtonText ({});
        addAndMakeVisible (band.enableToggle);
        band.enableAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            apvts, EQModule::bandParamID (idPrefix, i, "enabled"), band.enableToggle);

        // Q slider
        setupSlider (band.qSlider, juce::Colour (0xffb080f5));
        setupLabel  (band.qLabel, "Q");
        addAndMakeVisible (band.qSlider);
        addAndMakeVisible (band.qLabel);
        band.qAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, EQModule::bandParamID (idPrefix, i, "q"), band.qSlider);

        // Threshold slider
        setupSlider (band.threshSlider, juce::Colour (0xffe07840));
        setupLabel  (band.threshLabel, "Thr");
        addAndMakeVisible (band.threshSlider);
        addAndMakeVisible (band.threshLabel);
        band.threshAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, EQModule::bandParamID (idPrefix, i, "threshold"), band.threshSlider);

        // Ratio slider
        setupSlider (band.ratioSlider, juce::Colour (0xff40b080));
        setupLabel  (band.ratioLabel, "Rt");
        addAndMakeVisible (band.ratioSlider);
        addAndMakeVisible (band.ratioLabel);
        band.ratioAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, EQModule::bandParamID (idPrefix, i, "ratio"), band.ratioSlider);
    }
}

void EQControlsStrip::paint (juce::Graphics& g)
{
    // Background
    g.setColour (juce::Colour (0xff1a1a22));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);

    g.setColour (juce::Colour (0xffb080f5).withAlpha (0.25f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 4.0f, 1.0f);

    float colW = (float) getWidth() / (float) EQModule::maxBands;

    // Column headers and separators
    g.setFont (juce::Font (juce::FontOptions().withHeight (10.0f).withStyle ("Bold")));

    for (int i = 0; i < EQModule::maxBands; ++i)
    {
        float cx = (float) i * colW;

        // Column separator
        if (i > 0)
        {
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            g.drawLine (cx, 4.0f, cx, (float) getHeight() - 4.0f, 0.5f);
        }

        // Band number — full "Band N" now that each column has the width for it
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawText ("Band " + juce::String (i + 1), (int) cx + 6, 2, 70, 14,
                    juce::Justification::centredLeft);
    }
}

void EQControlsStrip::resized()
{
    constexpr int headerH = 16, sliderH = 18, gap = 3, labelW = 24;
    float colW = (float) getWidth() / (float) EQModule::maxBands;

    for (int i = 0; i < EQModule::maxBands; ++i)
    {
        auto& band = bands[(size_t) i];
        int   x    = (int) ((float) i * colW);
        int   w    = (int) colW - 4;

        // Enable toggle — right side of header, small square
        band.enableToggle.setBounds (x + w - 16, 1, 16, 14);

        int y = headerH;
        auto layoutRow = [&] (juce::Label& lbl, juce::Slider& sld)
        {
            lbl.setBounds (x + 3, y, labelW, sliderH);
            sld.setBounds (x + 3 + labelW, y, w - labelW - 3, sliderH);
            y += sliderH + gap;
        };

        layoutRow (band.qLabel,      band.qSlider);
        layoutRow (band.threshLabel, band.threshSlider);
        layoutRow (band.ratioLabel,  band.ratioSlider);
    }
}
