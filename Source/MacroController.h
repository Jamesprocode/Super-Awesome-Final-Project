/*
  ==============================================================================

    MacroController.h

    Lets a single "macro" parameter in the AudioProcessorValueTreeState
    drive any number of other parameters via a configurable mapping table.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

struct MacroMapping
{
    juce::String targetParamID;
    float minValue;
    float maxValue;
    float curve = 1.0f;
};

class MacroController : public juce::AudioProcessorValueTreeState::Listener
{
public:
    MacroController (juce::AudioProcessorValueTreeState& apvtsToUse,
                     const juce::String& macroParamID);
    ~MacroController() override;

    void addMapping (const MacroMapping& mapping);
    void setMappings (std::vector<MacroMapping> newMappings);
    void clearMappings();

    const std::vector<MacroMapping>& getMappings() const noexcept { return mappings; }

    void applyCurrentMacroValue();

    void parameterChanged (const juce::String& paramID, float newValue) override;

private:
    void applyMacro (float macroValue);

    juce::AudioProcessorValueTreeState& apvts;
    juce::String macroID;
    std::vector<MacroMapping> mappings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroController)
};
