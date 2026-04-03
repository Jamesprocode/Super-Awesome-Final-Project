/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
static void setupSectionLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area)
{
    g.setColour (juce::Colours::lightgrey);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (text, area, juce::Justification::centredLeft);
}

//==============================================================================
SuperAwesomeVocalChainAudioProcessorEditor::SuperAwesomeVocalChainAudioProcessorEditor (SuperAwesomeVocalChainAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (1000, 840);

    // Page label
    pageLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    pageLabel.setJustificationType (juce::Justification::centred);
    pageLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (pageLabel);

    // Nav buttons
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    prevButton.onClick = [this] { showPage (currentPage - 1); };
    nextButton.onClick = [this] { showPage (currentPage + 1); };

    for (auto& page : pages)
        addChildComponent (page);

    // ---- Page 1: Macro Knob ----
    macroKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    macroKnob.setRange (0.0, 1.0, 0.001);
    macroKnob.setValue (0.5);
    macroKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    pages[0].addAndMakeVisible (macroKnob);

    macroLabel.setText ("Macro", juce::dontSendNotification);
    macroLabel.setJustificationType (juce::Justification::centred);
    macroLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    macroLabel.setFont (juce::FontOptions (16.0f));
    pages[0].addAndMakeVisible (macroLabel);

    // ---- Page 3: All Parameters ----
    auto& apvts = audioProcessor.apvts;

    // EQ knobs
    setupKnob (lowFreqSlider,     lowFreqLabel,     "Low Freq");
    setupKnob (lowGainSlider,     lowGainLabel,     "Low Gain");
    setupKnob (lowQSlider,        lowQLabel,        "Low Q");
    setupKnob (lowMidFreqSlider,  lowMidFreqLabel,  "LMid Freq");
    setupKnob (lowMidGainSlider,  lowMidGainLabel,  "LMid Gain");
    setupKnob (lowMidQSlider,     lowMidQLabel,     "LMid Q");
    setupKnob (highMidFreqSlider, highMidFreqLabel, "HMid Freq");
    setupKnob (highMidGainSlider, highMidGainLabel, "HMid Gain");
    setupKnob (highMidQSlider,    highMidQLabel,    "HMid Q");
    setupKnob (highFreqSlider,    highFreqLabel,    "High Freq");
    setupKnob (highGainSlider,    highGainLabel,    "High Gain");
    setupKnob (highQSlider,       highQLabel,       "High Q");

    // Compressor knobs
    setupKnob (thresholdSlider, thresholdLabel, "Threshold");
    setupKnob (ratioSlider,     ratioLabel,     "Ratio");
    setupKnob (attackSlider,    attackLabel,    "Attack");
    setupKnob (releaseSlider,   releaseLabel,   "Release");

    //Chorus knobs
    setupKnob(chorusRateSlider, chorusRateLabel, "Rate");
    setupKnob(chorusDepthSlider, chorusDepthLabel, "Depth");
    setupKnob(chorusCenterDelaySlider, chorusCenterDelayLabel, "Center Delay");
    setupKnob(chorusFeedbackSlider, chorusFeedbackLabel, "Feedback");
    setupKnob(chorusMixSlider, chorusMixLabel, "Mix");

    // Reverb knobs
    setupKnob (roomSizeSlider, roomSizeLabel, "Room Size");
    setupKnob (dampingSlider,  dampingLabel,  "Damping");
    setupKnob (widthSlider,    widthLabel,    "Width");
    setupKnob (wetSlider,      wetLabel,      "Wet");
    setupKnob (drySlider,      dryLabel,      "Dry");

    freezeButton.setButtonText ("Freeze");
    freezeButton.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    parametersContent.addAndMakeVisible (freezeButton);

    // Attachments
    lowFreqAtt     = std::make_unique<SliderAttachment> (apvts, "lowFreq",     lowFreqSlider);
    lowGainAtt     = std::make_unique<SliderAttachment> (apvts, "lowGain",     lowGainSlider);
    lowQAtt        = std::make_unique<SliderAttachment> (apvts, "lowQ",        lowQSlider);
    lowMidFreqAtt  = std::make_unique<SliderAttachment> (apvts, "lowMidFreq",  lowMidFreqSlider);
    lowMidGainAtt  = std::make_unique<SliderAttachment> (apvts, "lowMidGain",  lowMidGainSlider);
    lowMidQAtt     = std::make_unique<SliderAttachment> (apvts, "lowMidQ",     lowMidQSlider);
    highMidFreqAtt = std::make_unique<SliderAttachment> (apvts, "highMidFreq", highMidFreqSlider);
    highMidGainAtt = std::make_unique<SliderAttachment> (apvts, "highMidGain", highMidGainSlider);
    highMidQAtt    = std::make_unique<SliderAttachment> (apvts, "highMidQ",    highMidQSlider);
    highFreqAtt    = std::make_unique<SliderAttachment> (apvts, "highFreq",    highFreqSlider);
    highGainAtt    = std::make_unique<SliderAttachment> (apvts, "highGain",    highGainSlider);
    highQAtt       = std::make_unique<SliderAttachment> (apvts, "highQ",       highQSlider);
    thresholdAtt   = std::make_unique<SliderAttachment> (apvts, "threshold",   thresholdSlider);
    ratioAtt       = std::make_unique<SliderAttachment> (apvts, "ratio",       ratioSlider);
    attackAtt      = std::make_unique<SliderAttachment> (apvts, "attack",      attackSlider);
    releaseAtt     = std::make_unique<SliderAttachment> (apvts, "release",     releaseSlider);
    roomSizeAtt    = std::make_unique<SliderAttachment> (apvts, "roomSize",    roomSizeSlider);
    dampingAtt     = std::make_unique<SliderAttachment> (apvts, "damping",     dampingSlider);
    widthAtt       = std::make_unique<SliderAttachment> (apvts, "width",       widthSlider);
    wetAtt         = std::make_unique<SliderAttachment> (apvts, "wet",         wetSlider);
    dryAtt         = std::make_unique<SliderAttachment> (apvts, "dry",         drySlider);
    freezeAtt      = std::make_unique<ButtonAttachment> (apvts, "freeze",      freezeButton);

    // Viewport setup
    parametersViewport.setViewedComponent (&parametersContent, false);
    parametersViewport.setScrollBarsShown (true, false);
    pages[2].addAndMakeVisible (parametersViewport);

    showPage (0);
}

SuperAwesomeVocalChainAudioProcessorEditor::~SuperAwesomeVocalChainAudioProcessorEditor()
{
}

void SuperAwesomeVocalChainAudioProcessorEditor::setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& name)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
    parametersContent.addAndMakeVisible (slider);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    label.setFont (juce::FontOptions (11.0f));
    parametersContent.addAndMakeVisible (label);
}

void SuperAwesomeVocalChainAudioProcessorEditor::showPage (int index)
{
    currentPage = juce::jlimit (0, numPages - 1, index);

    for (int i = 0; i < numPages; ++i)
        pages[i].setVisible (i == currentPage);

    pageLabel.setText (pageNames[currentPage], juce::dontSendNotification);
    prevButton.setEnabled (currentPage > 0);
    nextButton.setEnabled (currentPage < numPages - 1);

    repaint();
}

//==============================================================================
void SuperAwesomeVocalChainAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e2e));

    auto contentArea = getLocalBounds().reduced (10).withTrimmedTop (50).withTrimmedBottom (50);
    g.setColour (juce::Colour (0xff2e2e3e));
    g.fillRoundedRectangle (contentArea.toFloat(), 10.0f);

    if (currentPage == 1)
    {
        g.setColour (juce::Colours::grey);
        g.setFont (juce::FontOptions (14.0f));
        g.drawFittedText ("Macro Mapping controls go here", contentArea, juce::Justification::centred, 1);
    }
}

void SuperAwesomeVocalChainAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    pageLabel.setBounds (bounds.removeFromTop (40));

    auto navArea = bounds.removeFromBottom (40);
    prevButton.setBounds (navArea.removeFromLeft (80));
    nextButton.setBounds (navArea.removeFromRight (80));

    for (auto& page : pages)
        page.setBounds (bounds);

    // Page 1: big knob centred
    {
        auto area = pages[0].getLocalBounds();
        int knobSize = 200;
        macroKnob.setBounds (area.getCentreX() - knobSize / 2,
                             area.getCentreY() - knobSize / 2,
                             knobSize, knobSize);
        macroLabel.setBounds (area.getCentreX() - 60, macroKnob.getBottom() + 5, 120, 24);
    }

    // Page 3: scrollable parameter grid
    {
        auto area = pages[2].getLocalBounds();
        parametersViewport.setBounds (area);

        const int knobSize  = 70;
        const int labelH    = 16;
        const int cellW     = 80;
        const int cellH     = knobSize + labelH + 8;
        const int sectionH  = 24;
        const int cols      = 4;
        const int padX      = 10;
        const int padY      = 8;

        int y = padY;

        auto placeRow = [&] (std::vector<std::pair<juce::Slider*, juce::Label*>> items)
        {
            int x = padX;
            for (auto [slider, label] : items)
            {
                slider->setBounds (x + (cellW - knobSize) / 2, y, knobSize, knobSize);
                label->setBounds  (x, y + knobSize, cellW, labelH);
                x += cellW;
            }
            y += cellH;
        };

        // EQ section header
        y += sectionH; // space for section label drawn in paint (future)

        // EQ rows: 3 knobs per band, 4 bands = 3 rows of 4
        placeRow ({ {&lowFreqSlider, &lowFreqLabel}, {&lowMidFreqSlider, &lowMidFreqLabel},
                    {&highMidFreqSlider, &highMidFreqLabel}, {&highFreqSlider, &highFreqLabel} });
        placeRow ({ {&lowGainSlider, &lowGainLabel}, {&lowMidGainSlider, &lowMidGainLabel},
                    {&highMidGainSlider, &highMidGainLabel}, {&highGainSlider, &highGainLabel} });
        placeRow ({ {&lowQSlider, &lowQLabel}, {&lowMidQSlider, &lowMidQLabel},
                    {&highMidQSlider, &highMidQLabel}, {&highQSlider, &highQLabel} });

        y += sectionH;

        // Compressor row
        placeRow ({ {&thresholdSlider, &thresholdLabel}, {&ratioSlider, &ratioLabel},
                    {&attackSlider, &attackLabel}, {&releaseSlider, &releaseLabel} });

        y += sectionH;

        //Chorus row
        placeRow({ {&chorusRateSlider, &chorusRateLabel}, {&chorusDepthSlider, &chorusDepthLabel},
                    {&chorusCenterDelaySlider, &chorusCenterDelayLabel}, {&chorusFeedbackSlider, &chorusFeedbackLabel},
                    {&chorusMixSlider, &chorusMixLabel} });

        y += sectionH;

        // Reverb row
        placeRow ({ {&roomSizeSlider, &roomSizeLabel}, {&dampingSlider, &dampingLabel},
                    {&widthSlider, &widthLabel}, {&wetSlider, &wetLabel} });
        placeRow ({ {&drySlider, &dryLabel} });

        freezeButton.setBounds (padX, y, 100, 24);
        y += 30;

        parametersContent.setSize (area.getWidth(), y + padY);
    }
}
