/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "PluginProcessor.h"

//==============================================================================
class SuperAwesomeVocalChainAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SuperAwesomeVocalChainAudioProcessorEditor (SuperAwesomeVocalChainAudioProcessor&);
    ~SuperAwesomeVocalChainAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SuperAwesomeVocalChainAudioProcessor& audioProcessor;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Page navigation
    int currentPage = 0;
    static constexpr int numPages = 3;
    const juce::String pageNames[numPages] = { "Macro Control", "Macro Mapping", "All Parameters" };

    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::Label pageLabel;

    juce::Component pages[numPages];

    // Page 1 - Macro Control
    juce::Slider macroKnob;
    juce::Label  macroLabel;

    // Page 3 - All Parameters (scrollable)
    juce::Viewport parametersViewport;
    juce::Component parametersContent;

    // EQ
    juce::Slider lowFreqSlider,    lowGainSlider,    lowQSlider;
    juce::Slider lowMidFreqSlider, lowMidGainSlider, lowMidQSlider;
    juce::Slider highMidFreqSlider,highMidGainSlider,highMidQSlider;
    juce::Slider highFreqSlider,   highGainSlider,   highQSlider;

    juce::Label lowFreqLabel,    lowGainLabel,    lowQLabel;
    juce::Label lowMidFreqLabel, lowMidGainLabel, lowMidQLabel;
    juce::Label highMidFreqLabel,highMidGainLabel,highMidQLabel;
    juce::Label highFreqLabel,   highGainLabel,   highQLabel;

    // Compressor
    juce::Slider thresholdSlider, ratioSlider, attackSlider, releaseSlider;
    juce::Label  thresholdLabel,  ratioLabel,  attackLabel,  releaseLabel;

    // Reverb
    juce::Slider roomSizeSlider, dampingSlider, widthSlider, wetSlider, drySlider;
    juce::Label  roomSizeLabel,  dampingLabel,  widthLabel,  wetLabel,  dryLabel;
    juce::ToggleButton freezeButton;

    // Attachments
    std::unique_ptr<SliderAttachment> lowFreqAtt,    lowGainAtt,    lowQAtt;
    std::unique_ptr<SliderAttachment> lowMidFreqAtt, lowMidGainAtt, lowMidQAtt;
    std::unique_ptr<SliderAttachment> highMidFreqAtt,highMidGainAtt,highMidQAtt;
    std::unique_ptr<SliderAttachment> highFreqAtt,   highGainAtt,   highQAtt;
    std::unique_ptr<SliderAttachment> thresholdAtt,  ratioAtt,      attackAtt, releaseAtt;
    std::unique_ptr<SliderAttachment> roomSizeAtt,   dampingAtt,    widthAtt,  wetAtt, dryAtt;
    std::unique_ptr<ButtonAttachment> freezeAtt;

    void setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& name);
    void showPage (int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SuperAwesomeVocalChainAudioProcessorEditor)
};
