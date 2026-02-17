/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
*/
class TestReverbAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    TestReverbAudioProcessor();
    ~TestReverbAudioProcessor() override;

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
    juce::AudioProcessorValueTreeState apvts;

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
};
