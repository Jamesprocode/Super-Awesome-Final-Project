/*
  ==============================================================================

    MacroController.cpp

  ==============================================================================
*/

#include "MacroController.h"

MacroController::MacroController (juce::AudioProcessorValueTreeState& apvtsToUse,
                                  const juce::String& macroParamID)
    : apvts (apvtsToUse), macroID (macroParamID)
{
    apvts.addParameterListener (macroID, this);
}

MacroController::~MacroController()
{
    apvts.removeParameterListener (macroID, this);
}

void MacroController::addMapping (const MacroMapping& mapping)
{
    mappings.push_back (mapping);
    applyCurrentMacroValue();
}

void MacroController::setMappings (std::vector<MacroMapping> newMappings)
{
    mappings = std::move (newMappings);
    applyCurrentMacroValue();
}

void MacroController::clearMappings()
{
    mappings.clear();
}

void MacroController::applyCurrentMacroValue()
{
    if (auto* raw = apvts.getRawParameterValue (macroID))
        applyMacro (raw->load());
}

void MacroController::parameterChanged (const juce::String& paramID, float newValue)
{
    if (paramID == macroID)
        applyMacro (newValue);
}

void MacroController::applyMacro (float macroValue)
{
    const float x = juce::jlimit (0.0f, 1.0f, macroValue);

    for (const auto& m : mappings)
    {
        auto* param = apvts.getParameter (m.targetParamID);
        if (param == nullptr)
            continue;

        const float shaped = std::pow (x, m.curve);
        const float target = juce::jmap (shaped, m.minValue, m.maxValue);
        const auto& range = apvts.getParameterRange (m.targetParamID);
        const float clamped = juce::jlimit (range.start, range.end, target);
        param->setValueNotifyingHost (range.convertTo0to1 (clamped));
    }
}
