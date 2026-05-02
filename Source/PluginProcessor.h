/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "MacroController.h"
#include "ParameterListener.h"

//==============================================================================
/**
*/
class SuperAwesomeVocalChainAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SuperAwesomeVocalChainAudioProcessor();
    ~SuperAwesomeVocalChainAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    std::unique_ptr<juce::AudioProcessorValueTreeState> apvts;

    std::unique_ptr<MacroController> macroController;

    juce::String lastPresetName { "Default" };

    /** Peak levels (linear 0..1) for input/output VU meters. Read on message thread, written on audio thread. */
    std::atomic<float> meterInputPeak  { 0.0f };
    std::atomic<float> meterOutputPeak { 0.0f };

private:
    //==============================================================================
    // juce::AudioParameterFloat* roomSizeParam;
    // juce::AudioParameterFloat* dampingParam;
    // juce::AudioParameterFloat* widthParam;
    // juce::AudioParameterFloat* wetParam;
    // juce::AudioParameterFloat* dryParam;
    // juce::AudioParameterBool* freezeParam;
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    juce::dsp::ProcessorDuplicator<Filter, Coefficients> lowShelfFilter, lowMidPeakFilter, highMidPeakFilter, highShelfFilter;


    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;

    // Create the compressor
    juce::dsp::Compressor<float> comp;

    //Create the chorus
    juce::dsp::Chorus<float> chorus;

    //Create Saturator
    juce::dsp::ProcessorChain<
        juce::dsp::Gain<float>,
        juce::dsp::WaveShaper<float>,
        juce::dsp::Gain<float>
    > saturator;

    //Parameter listener for process block
    std::atomic<bool> eqNeedsUpdate{ true };
    std::atomic<bool> compNeedsUpdate{ true };
    std::atomic<bool> satNeedsUpdate{ true };
    std::atomic<bool> revNeedsUpdate{ true };
    std::atomic<bool> chorNeedsUpdate{ true };

    std::unique_ptr<ParameterListener> listener;
    std::atomic<bool> isPrepared{ false };

    /** Post–input-trim dry signal per channel for parallel output dry/wet mix */
    juce::AudioBuffer<float> dryWetBuffer;
};
