/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "melatonin_inspector/melatonin_inspector.h"
#include "PluginProcessor.h"

//==============================================================================
class SuperAwesomeVocalChainAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      public juce::Timer
{
public:
    SuperAwesomeVocalChainAudioProcessorEditor (SuperAwesomeVocalChainAudioProcessor&);
    ~SuperAwesomeVocalChainAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SuperAwesomeVocalChainAudioProcessor& audioProcessor;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    // All your UI lives inside this content component
    juce::Component content;

    // The three tabs and their buttons
    juce::Component macroPage;
    juce::Component mapPage;
    juce::Component detailPage;

    juce::TextButton macroTab { "MACRO" };
    juce::TextButton mapTab { "MAPPING" };
    juce::TextButton detailTab { "DETAILED" };

    void showPage(int index);

    // Macro page components
    juce::Slider macroKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> macroKnobAttachment;

    // Detailed page components
    juce::Viewport detailViewport;
    juce::Component detailContent;

    // EQ parameters
    juce::Slider lowFreqSlider, lowGainSlider, lowQSlider;
    juce::Slider lowMidFreqSlider, lowMidGainSlider, lowMidQSlider;
    juce::Slider highMidFreqSlider, highMidGainSlider, highMidQSlider;
    juce::Slider highFreqSlider, highGainSlider, highQSlider;

    juce::Label eqLabel;
    juce::Label lowFreqLabel, lowGainLabel, lowQLabel;
    juce::Label lowMidFreqLabel, lowMidGainLabel, lowMidQLabel;
    juce::Label highMidFreqLabel, highMidGainLabel, highMidQLabel;
    juce::Label highFreqLabel, highGainLabel, highQLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowFreqAttach, lowGainAttach, lowQAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowMidFreqAttach, lowMidGainAttach, lowMidQAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highMidFreqAttach, highMidGainAttach, highMidQAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highFreqAttach, highGainAttach, highQAttach;

    // Compressor parameters
    juce::Slider thresholdSlider, ratioSlider, attackSlider, releaseSlider;
    juce::Label compLabel;
    juce::Label thresholdLabel, ratioLabel, attackLabel, releaseLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttach, ratioAttach, attackAttach, releaseAttach;

    // Saturator parameters
    juce::Slider preGainSlider, postGainSlider;
    juce::Label saturateLabel;
    juce::Label preGainLabel, postGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> preGainAttach, postGainAttach;

    // Chorus parameters
    juce::Slider lfoRateSlider, lfoDepthSlider, centerDelaySlider, chorusFeedbackSlider, chorusMixSlider;
    juce::Label chorusLabel;
    juce::Label lfoRateLabel, lfoDepthLabel, centerDelayLabel, chorusFeedbackLabel, chorusMixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoRateAttach, lfoDepthAttach, centerDelayAttach, chorusFeedbackAttach, chorusMixAttach;

    // Reverb parameters
    juce::Slider roomSizeSlider, dampingSlider, reverbWidthSlider, reverbWetSlider, reverbDrySlider, freezeModeSlider;
    juce::Label reverbLabel;
    juce::Label roomSizeLabel, dampingLabel, reverbWidthLabel, reverbWetLabel, reverbDryLabel, freezeModeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> roomSizeAttach, dampingAttach, reverbWidthAttach, reverbWetAttach, reverbDryAttach, freezeModeAttach;

    // Inspector inspects `content`, not the whole editor (and not itself)
    std::unique_ptr<melatonin::Inspector> inspector;

    void updateVisibility();
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SuperAwesomeVocalChainAudioProcessorEditor)
};