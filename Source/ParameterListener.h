/*
  ==============================================================================

    ParameterListener.h

    Used in the process block function to listen for parameter changes for specific effects.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

class ParameterListener : public juce::AudioProcessorValueTreeState::Listener
{
public:
    ParameterListener(std::atomic<bool>& eq,
        std::atomic<bool>& comp,
        std::atomic<bool>& sat,
        std::atomic<bool>& rev,
        std::atomic<bool>& chor)
        : eqNeedsUpdate(eq),
        compNeedsUpdate(comp),
        satNeedsUpdate(sat),
        revNeedsUpdate(rev),
        chorNeedsUpdate(chor)
    {
    }

    void parameterChanged(const juce::String& paramID, float) override
    {
        if (isEQParam(paramID))        eqNeedsUpdate = true;
        else if (isCompParam(paramID)) compNeedsUpdate = true;
        else if (isSatParam(paramID))  satNeedsUpdate = true;
        else if (isRevParam(paramID))  revNeedsUpdate = true;
        else if (isChorParam(paramID)) chorNeedsUpdate = true;
    }

private:
    std::atomic<bool>& eqNeedsUpdate;
    std::atomic<bool>& compNeedsUpdate;
    std::atomic<bool>& satNeedsUpdate;
    std::atomic<bool>& revNeedsUpdate;
    std::atomic<bool>& chorNeedsUpdate;

    bool isEQParam(const juce::String& id)
    {
        return id == "lowFreq" || id == "lowGain" || id == "lowQ" ||
            id == "lowMidFreq" || id == "lowMidGain" || id == "lowMidQ" ||
            id == "highMidFreq" || id == "highMidGain" || id == "highMidQ" ||
            id == "highFreq" || id == "highGain" || id == "highQ";
    }

    bool isCompParam(const juce::String& id)
    {
        return id == "threshold" || id == "ratio" ||
            id == "attack" || id == "release";
    }

    bool isSatParam(const juce::String& id)
    {
        return id == "preGain" || id == "postGain";
    }

    bool isRevParam(const juce::String& id)
    {
        return id == "roomSize" || id == "damping" || id == "width" ||
            id == "wet" || id == "dry" || id == "freeze";
    }

    bool isChorParam(const juce::String& id)
    {
        return id == "lforate" || id == "lfodepth" ||
            id == "centerdelay" || id == "chorfeedback" ||
            id == "chormix";
    }
};
