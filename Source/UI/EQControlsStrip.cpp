#include "EQControlsStrip.h"

void EQControlsStrip::setupSlider (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::LinearBar);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setColour (juce::Slider::trackColourId,    juce::Colour (0xff3a3a4a));
    s.setColour (juce::Slider::thumbColourId,    juce::Colour (0xffb080f5));
    s.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff222230));
}

EQControlsStrip::EQControlsStrip (juce::AudioProcessorValueTreeState& apvts)
{
    for (int i = 0; i < EQModule::maxBands; ++i)
    {
        auto& band = bands[(size_t) i];

        // Enable toggle
        band.enableToggle.setButtonText ({});
        addAndMakeVisible (band.enableToggle);
        band.enableAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            apvts, EQModule::bandParamID (i, "enabled"), band.enableToggle);

        // Q slider
        setupSlider (band.qSlider);
        addAndMakeVisible (band.qSlider);
        band.qAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, EQModule::bandParamID (i, "q"), band.qSlider);

        // Threshold slider
        setupSlider (band.threshSlider);
        band.threshSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffe07840));
        addAndMakeVisible (band.threshSlider);
        band.threshAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, EQModule::bandParamID (i, "threshold"), band.threshSlider);

        // Ratio slider
        setupSlider (band.ratioSlider);
        band.ratioSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xff40b080));
        addAndMakeVisible (band.ratioSlider);
        band.ratioAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, EQModule::bandParamID (i, "ratio"), band.ratioSlider);
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

    // Column headers and row labels
    g.setFont (juce::Font (juce::FontOptions().withHeight (8.5f).withStyle ("Bold")));

    for (int i = 0; i < EQModule::maxBands; ++i)
    {
        float cx = (float) i * colW;

        // Column separator
        if (i > 0)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.drawLine (cx, 4.0f, cx, (float) getHeight() - 4.0f, 0.5f);
        }

        // Band number
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawText ("B" + juce::String (i + 1), (int) cx + 2, 3, 22, 12,
                    juce::Justification::centredLeft);
    }

    // Row labels (painted once on the left of the first column's sliders)
    constexpr int headerH = 18, sliderH = 16, gap = 4;
    int y = headerH;
    g.setFont (juce::Font (juce::FontOptions().withHeight (8.0f)));
    for (auto* label : { "Q", "Thr", "Ratio" })
    {
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawText (label, 2, y, 28, sliderH, juce::Justification::centredLeft);
        y += sliderH + gap;
    }
}

void EQControlsStrip::resized()
{
    constexpr int headerH = 18, sliderH = 16, gap = 4, labelW = 30;
    float colW = (float) getWidth() / (float) EQModule::maxBands;

    for (int i = 0; i < EQModule::maxBands; ++i)
    {
        auto& band = bands[(size_t) i];
        int   x    = (int) ((float) i * colW);
        int   w    = (int) colW - 2;

        // Enable toggle — right side of header, small square
        band.enableToggle.setBounds (x + w - 16, 2, 16, 14);

        // Sliders — full column width, but leave space for the label column on band 0
        int sliderX = (i == 0) ? x + labelW : x + 2;
        int sliderW = (i == 0) ? w - labelW : w - 2;
        int y = headerH;

        band.qSlider.setBounds    (sliderX, y, sliderW, sliderH); y += sliderH + gap;
        band.threshSlider.setBounds (sliderX, y, sliderW, sliderH); y += sliderH + gap;
        band.ratioSlider.setBounds  (sliderX, y, sliderW, sliderH);
    }
}
