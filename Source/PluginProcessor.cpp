/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SuperAwesomeVocalChainAudioProcessor::SuperAwesomeVocalChainAudioProcessor()
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

}

SuperAwesomeVocalChainAudioProcessor::~SuperAwesomeVocalChainAudioProcessor()
{
}

//==============================================================================
const juce::String SuperAwesomeVocalChainAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SuperAwesomeVocalChainAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SuperAwesomeVocalChainAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SuperAwesomeVocalChainAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SuperAwesomeVocalChainAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SuperAwesomeVocalChainAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SuperAwesomeVocalChainAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SuperAwesomeVocalChainAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SuperAwesomeVocalChainAudioProcessor::getProgramName (int index)
{
    return {};
}

void SuperAwesomeVocalChainAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SuperAwesomeVocalChainAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    lowShelfFilter.prepare(spec);
    lowMidPeakFilter.prepare(spec);
    highMidPeakFilter.prepare(spec);
    highShelfFilter.prepare(spec);

    reverb.prepare(spec);

    comp.prepare(spec);

    chorus.prepare(spec);

    saturator.get<1>().functionToUse = [](float x) noexcept {
        return std::tanh(x); // set waveshaping to hyperbolic tangent soft clipping
        };

    saturator.prepare(spec);
}

void SuperAwesomeVocalChainAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SuperAwesomeVocalChainAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SuperAwesomeVocalChainAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    auto sampleRate = getSampleRate();

    *lowShelfFilter.state = *Coefficients::makeLowShelf(sampleRate, apvts.getRawParameterValue("lowFreq")->load(), apvts.getRawParameterValue("lowQ")->load(), juce::Decibels::decibelsToGain(apvts.getRawParameterValue("lowGain")->load()));
    *lowMidPeakFilter.state = *Coefficients::makePeakFilter(sampleRate, apvts.getRawParameterValue("lowMidFreq")->load(), apvts.getRawParameterValue("lowMidQ")->load(), juce::Decibels::decibelsToGain(apvts.getRawParameterValue("lowMidGain")->load()));
    *highMidPeakFilter.state = *Coefficients::makePeakFilter(sampleRate, apvts.getRawParameterValue("highMidFreq")->load(), apvts.getRawParameterValue("highMidQ")->load(), juce::Decibels::decibelsToGain(apvts.getRawParameterValue("highMidGain")->load()));
    *highShelfFilter.state = *Coefficients::makeHighShelf(sampleRate, apvts.getRawParameterValue("highFreq")->load(), apvts.getRawParameterValue("highQ")->load(), juce::Decibels::decibelsToGain(apvts.getRawParameterValue("highGain")->load()));

    // Update compressor parameters
    comp.setThreshold(*apvts.getRawParameterValue("threshold"));
    comp.setRatio(*apvts.getRawParameterValue("ratio"));
    comp.setAttack(*apvts.getRawParameterValue("attack"));
    comp.setRelease(*apvts.getRawParameterValue("release"));


    // Update reverb parameters
    reverbParams.roomSize   = *apvts.getRawParameterValue("roomSize");
    reverbParams.damping    = *apvts.getRawParameterValue("damping");
    reverbParams.width      = *apvts.getRawParameterValue("width");
    reverbParams.wetLevel   = *apvts.getRawParameterValue("wet");
    reverbParams.dryLevel   = *apvts.getRawParameterValue("dry");
    reverbParams.freezeMode = *apvts.getRawParameterValue("freeze");

    reverb.setParameters (reverbParams);


    //Update chorus parameters
    chorus.setRate(*apvts.getRawParameterValue("lforate"));
    chorus.setDepth(*apvts.getRawParameterValue("lfodepth"));
    chorus.setCentreDelay(*apvts.getRawParameterValue("centerdelay"));
    chorus.setFeedback(*apvts.getRawParameterValue("chorfeedback"));
    chorus.setMix(*apvts.getRawParameterValue("chormix"));

    //Update saturator parameters
    saturator.get<0>().setGainLinear(*apvts.getRawParameterValue("preGain"));
    saturator.get<2>().setGainLinear(*apvts.getRawParameterValue("postGain"));

    // Process the entire buffer at once (stereo)
    // juce::dsp::AudioBlock<float> block (buffer);
    // juce::dsp::ProcessContextReplacing<float> context (block);
    //process eq
    lowShelfFilter.process(context);
    lowMidPeakFilter.process(context);
    highMidPeakFilter.process(context);
    highShelfFilter.process(context);
    //process saturator
    saturator.process(context);
    //process reverb
    reverb.process (context);
    //process chorus
    chorus.process(context);
    //process compressor
    comp.process(context);
}

//==============================================================================
bool SuperAwesomeVocalChainAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SuperAwesomeVocalChainAudioProcessor::createEditor()
{
    return new SuperAwesomeVocalChainAudioProcessorEditor (*this);
}

//==============================================================================
void SuperAwesomeVocalChainAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SuperAwesomeVocalChainAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout SuperAwesomeVocalChainAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Create parameters for the EQ effect
    // Low Band (Low Shelf)
    layout.add(std::make_unique<juce::AudioParameterFloat>("lowFreq", "Low Freq", 20.0f, 500.0f, 200.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("lowGain", "Low Gain", -24.0f, 24.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("lowQ", "Low Q", 0.1f, 10.0f, 0.707f));

    // Low-Mid Band (Peak)
    layout.add(std::make_unique<juce::AudioParameterFloat>("lowMidFreq", "Low-Mid Freq", 200.0f, 2000.0f, 500.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("lowMidGain", "Low-Mid Gain", -24.0f, 24.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("lowMidQ", "Low-Mid Q", 0.1f, 10.0f, 0.707f));

    // High-Mid Band (Peak)
    layout.add(std::make_unique<juce::AudioParameterFloat>("highMidFreq", "High-Mid Freq", 2000.0f, 8000.0f, 3000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("highMidGain", "High-Mid Gain", -24.0f, 24.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("highMidQ", "High-Mid Q", 0.1f, 10.0f, 0.707f));

    // High Band (High Shelf)
    layout.add(std::make_unique<juce::AudioParameterFloat>("highFreq", "High Freq", 5000.0f, 20000.0f, 10000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("highGain", "High Gain", -24.0f, 24.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("highQ", "High Q", 0.1f, 10.0f, 0.707f));

    // Create parameters for the reverb effect
    layout.add(std::make_unique<juce::AudioParameterFloat> ("roomSize", "Room Size", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat> ("damping", "Damping", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat> ("width", "Width", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat> ("wet", "Wet Level", 0.0f, 1.0f, 0.33f));
    layout.add(std::make_unique<juce::AudioParameterFloat> ("dry", "Dry Level", 0.0f, 1.0f, 0.67f));
    layout.add(std::make_unique<juce::AudioParameterBool> ("freeze", "Freeze", false));

    // Create parameter for compressor effect
    layout.add(std::make_unique<juce::AudioParameterFloat> ("threshold", "Threshold", -60.0f, 0.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat> ("ratio", "Ratio", 1.0f, 10.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat> ("attack", "Attack", 2.0f, 200.0f, 2.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat> ("release", "Release", 30.0f, 1000.0f, 30.0f));

    //Create parameters for Chorus effect
    layout.add(std::make_unique<juce::AudioParameterFloat> ("lforate", "Rate",   0.0f, 100.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("lfodepth", "Depth", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("centerdelay", "Center Delay", 1.0f, 100.0f, 10.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("chorfeedback", "Feedback", -1.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("chormix", "Mix", 0.0f, 1.0f, 0.5f));

    //Create parameters for Saturator
    layout.add(std::make_unique<juce::AudioParameterFloat>("preGain", "Pre-Gain", 0.0f, 5.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("postGain", "Post-Gain", 0.0f, 1.0f, 0.5f));


    return layout;
}
//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SuperAwesomeVocalChainAudioProcessor();
}