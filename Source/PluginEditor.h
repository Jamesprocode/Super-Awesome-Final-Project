/*
  ==============================================================================

    This file contains the basic framework code for a plugin editor.

  ==============================================================================
*/

#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include "melatonin_inspector/melatonin_inspector.h"
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

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

    // All your UI lives inside this component
    juce::Component content;

    juce::TextButton macroTab { "MACRO" };
    juce::TextButton mapTab { "MAPPING" };
    juce::TextButton detailTab { "DETAILED" };

    void showPage (int index);
    static constexpr int macroPageIndex = 0;
    static constexpr int mapPageIndex = 1;
    static constexpr int detailedPageIndex = 2;

    // Inspector inspects `content`
    std::unique_ptr<melatonin::Inspector> inspector;

    using Resource = juce::WebBrowserComponent::Resource;

    juce::WebBrowserComponent::Options buildWebViewOptions();
    std::optional<Resource> getResource (const juce::String& url);
    juce::String getMappingStateJson() const;


    std::vector<std::unique_ptr<juce::WebSliderRelay>> webSliderRelays;
    std::vector<std::unique_ptr<juce::WebToggleButtonRelay>> webToggleRelays;

    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> webSliderAttachments;
    std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>> webToggleAttachments;

    std::unique_ptr<juce::WebBrowserComponent> webView;

    void updateVisibility();
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SuperAwesomeVocalChainAudioProcessorEditor)
};
