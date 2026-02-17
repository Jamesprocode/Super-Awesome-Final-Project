/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TestReverbAudioProcessor::TestReverbAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
        apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    // addParameter(roomSizeParam = new juce::AudioParameterFloat(
    // "roomSize", "Room Size", 0.0f, 1.0f, 0.5f));
    //
    // addParameter(dampingParam = new juce::AudioParameterFloat(
    //     "damping", "Damping", 0.0f, 1.0f, 0.5f));
    //
    // addParameter(widthParam = new juce::AudioParameterFloat(
    //     "width", "Width", 0.0f, 1.0f, 1.0f));
    //
    // addParameter(wetParam = new juce::AudioParameterFloat(
    //     "wet", "Wet Level", 0.0f, 1.0f, 0.33f));
    //
    // addParameter(dryParam = new juce::AudioParameterFloat(
    //     "dry", "Dry Level", 0.0f, 1.0f, 0.67f));
    //
    // addParameter(freezeParam = new juce::AudioParameterBool(
    //     "freeze", "Freeze", false));


}

TestReverbAudioProcessor::~TestReverbAudioProcessor()
{
}

//==============================================================================
const juce::String TestReverbAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TestReverbAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TestReverbAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TestReverbAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TestReverbAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TestReverbAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int TestReverbAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TestReverbAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String TestReverbAudioProcessor::getProgramName (int index)
{
    return {};
}

void TestReverbAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void TestReverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    reverb.prepare(spec);
}

void TestReverbAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TestReverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void TestReverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update reverb parameters
    reverbParams.roomSize   = *apvts.getRawParameterValue("roomSize");
    reverbParams.damping    = *apvts.getRawParameterValue("roomSize");
    reverbParams.width      = *apvts.getRawParameterValue("roomSize");
    reverbParams.wetLevel   = *apvts.getRawParameterValue("roomSize");
    reverbParams.dryLevel   = *apvts.getRawParameterValue("roomSize");
    reverbParams.freezeMode = *apvts.getRawParameterValue("roomSize");

    reverb.setParameters (reverbParams);

    // Process the entire buffer at once (stereo)
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    reverb.process (context);
}

//==============================================================================
bool TestReverbAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TestReverbAudioProcessor::createEditor()
{
    // return new TestReverbAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void TestReverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void TestReverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout TestReverbAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Create parameters for the reverb effect
    layout.add(std::make_unique<juce::AudioParameterFloat> ("roomSize", "Room Size", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat> ("damping", "Damping", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat> ("width", "Width", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat> ("wet", "Wet Level", 0.0f, 1.0f, 0.33f));
    layout.add(std::make_unique<juce::AudioParameterFloat> ("dry", "Dry Level", 0.0f, 1.0f, 0.67f));
    layout.add(std::make_unique<juce::AudioParameterBool> ("freeze", "Freeze", false));



    return layout;
}
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TestReverbAudioProcessor();
}
