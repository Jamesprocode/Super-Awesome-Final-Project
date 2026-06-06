/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/




#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>
#include <cmath>

namespace {

/** Factorials for n in [0 … 5] — factorial(0) == 1 for unrank indexing. */
int factLookup (int n)
{
    static const int tbl[] { 1, 1, 2, 6, 24, 120 };
    const int i = juce::jlimit (0, 5, n);
    return tbl[i];
}

enum class FxEffectId : uint8_t {
    Eq     = 0,
    Comp   = 1,
    Saturator = 2,
    Chorus = 3,
    Reverb = 4
};

/** Converts 0 … 119 to one of the 5! effect permutations (lex/unrank convention shared with UI). */
void permutationIndexToOrder (int rawIndex, std::array<uint8_t, 5>& out)
{
    const int index = juce::jlimit (0, 119, rawIndex);
    uint8_t pool[] { 0, 1, 2, 3, 4 };
    int poolSize = 5;
    int k = index;

    for (int pi = 0; pi < 5; ++pi)
    {
        const int f = factLookup (poolSize - 1);
        jassert (f > 0);
        const int pos = k / f;
        k %= f;
        jassert (pos >= 0 && pos < poolSize);

        out[(size_t) pi] = pool[pos];

        for (int j = pos + 1; j < poolSize; ++j)
            pool[(size_t) (j - 1)] = pool[j];

        --poolSize;
    }
}

float safeFilterFrequency (float rawHz, double sampleRate)
{
    const auto maxHz = static_cast<float> (juce::jmax (20.0, sampleRate * 0.45));
    return juce::jlimit (20.0f, maxHz, rawHz);
}

} // namespace

//==============================================================================
SuperAwesomeVocalChainAudioProcessor::SuperAwesomeVocalChainAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
    apvts = std::make_unique<juce::AudioProcessorValueTreeState>(
        *this, nullptr, "Parameters", createParameterLayout()
    );

    macroController = std::make_unique<MacroController>(*apvts, "macro");

    listener = std::make_unique<ParameterListener>(
        eqNeedsUpdate,
        compNeedsUpdate,
        satNeedsUpdate,
        revNeedsUpdate,
        chorNeedsUpdate
    );

    //Add parameter listener to the tree for all parameters
    // EQ
    
    // EQ
    apvts->addParameterListener("lowFreq", listener.get());
    apvts->addParameterListener("lowGain", listener.get());
    apvts->addParameterListener("lowQ", listener.get());

    apvts->addParameterListener("lowMidFreq", listener.get());
    apvts->addParameterListener("lowMidGain", listener.get());
    apvts->addParameterListener("lowMidQ", listener.get());

    apvts->addParameterListener("highMidFreq", listener.get());
    apvts->addParameterListener("highMidGain", listener.get());
    apvts->addParameterListener("highMidQ", listener.get());

    apvts->addParameterListener("highFreq", listener.get());
    apvts->addParameterListener("highGain", listener.get());
    apvts->addParameterListener("highQ", listener.get());

    // Compressor
    apvts->addParameterListener("threshold", listener.get());
    apvts->addParameterListener("ratio", listener.get());
    apvts->addParameterListener("attack", listener.get());
    apvts->addParameterListener("release", listener.get());

    // Reverb
    apvts->addParameterListener("roomSize", listener.get());
    apvts->addParameterListener("damping", listener.get());
    apvts->addParameterListener("width", listener.get());
    apvts->addParameterListener("wet", listener.get());
    apvts->addParameterListener("dry", listener.get());
    apvts->addParameterListener("freeze", listener.get());

    // Chorus
    apvts->addParameterListener("lforate", listener.get());
    apvts->addParameterListener("lfodepth", listener.get());
    apvts->addParameterListener("centerdelay", listener.get());
    apvts->addParameterListener("chorfeedback", listener.get());
    apvts->addParameterListener("chormix", listener.get());

    // Saturator
    apvts->addParameterListener("preGain", listener.get());
    apvts->addParameterListener("postGain", listener.get());
    apvts->addParameterListener("satType", listener.get());
}

SuperAwesomeVocalChainAudioProcessor::~SuperAwesomeVocalChainAudioProcessor()
{
    if (listener != nullptr)
    {
        // EQ
        apvts->removeParameterListener("lowFreq", listener.get());
        apvts->removeParameterListener("lowGain", listener.get());
        apvts->removeParameterListener("lowQ", listener.get());

        apvts->removeParameterListener("lowMidFreq", listener.get());
        apvts->removeParameterListener("lowMidGain", listener.get());
        apvts->removeParameterListener("lowMidQ", listener.get());

        apvts->removeParameterListener("highMidFreq", listener.get());
        apvts->removeParameterListener("highMidGain", listener.get());
        apvts->removeParameterListener("highMidQ", listener.get());

        apvts->removeParameterListener("highFreq", listener.get());
        apvts->removeParameterListener("highGain", listener.get());
        apvts->removeParameterListener("highQ", listener.get());

        // Compressor
        apvts->removeParameterListener("threshold", listener.get());
        apvts->removeParameterListener("ratio", listener.get());
        apvts->removeParameterListener("attack", listener.get());
        apvts->removeParameterListener("release", listener.get());

        // Reverb
        apvts->removeParameterListener("roomSize", listener.get());
        apvts->removeParameterListener("damping", listener.get());
        apvts->removeParameterListener("width", listener.get());
        apvts->removeParameterListener("wet", listener.get());
        apvts->removeParameterListener("dry", listener.get());
        apvts->removeParameterListener("freeze", listener.get());

        // Chorus
        apvts->removeParameterListener("lforate", listener.get());
        apvts->removeParameterListener("lfodepth", listener.get());
        apvts->removeParameterListener("centerdelay", listener.get());
        apvts->removeParameterListener("chorfeedback", listener.get());
        apvts->removeParameterListener("chormix", listener.get());

        // Saturator
        apvts->removeParameterListener("preGain", listener.get());
        apvts->removeParameterListener("postGain", listener.get());
        apvts->removeParameterListener("satType", listener.get());
    }
    listener.reset();
    macroController.reset();
    apvts.reset();
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
    juce::ignoreUnused (index);
}

const juce::String SuperAwesomeVocalChainAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void SuperAwesomeVocalChainAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void SuperAwesomeVocalChainAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());

    lowShelfFilter.prepare(spec);
    lowMidPeakFilter.prepare(spec);
    highMidPeakFilter.prepare(spec);
    highShelfFilter.prepare(spec);

    reverb.prepare(spec);

    comp.prepare(spec);

    chorus.prepare(spec);

    //set waveshaping function
    auto& waveshaper = saturator.get<1>();

    waveshaper.functionToUse = [this](float x) noexcept {
        switch (currentSatMode.load (std::memory_order_relaxed))
        {
            case 0:  return juce::jlimit (-1.0f, 1.0f, x - (x * x * x) / 3.0f);   // Cubic — least saturation
            case 2:  return x / (1.0f + 0.5f * std::abs (x));                     // Tape
            case 3:  return std::tanh (x + 0.2f * x * x);                         // Tube — asymmetric
            case 4:  return juce::jlimit (-1.0f, 1.0f, x);                        // Hard — clipping
            case 1:
            default: return std::tanh (x);                                        // Soft (default)
        }
    };

    saturator.prepare(spec);

    dryWetBuffer.setSize ((int) spec.numChannels, (int) samplesPerBlock, false, false, true);

    //lowShelfFilter.state = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, 1000.0f, 0.707f, 1.0f);
    //lowMidPeakFilter.state = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 1000.0f, 0.707f, 1.0f);
    //highMidPeakFilter.state = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 2000.0f, 0.707f, 1.0f);
    //highShelfFilter.state = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 5000.0f, 0.707f, 1.0f);

    const auto eqFreq = [this, sampleRate] (const char* paramID)
    {
        return safeFilterFrequency (apvts->getRawParameterValue (paramID)->load(), sampleRate);
    };

    *lowShelfFilter.state = *Coefficients::makeLowShelf(sampleRate, eqFreq("lowFreq"), apvts->getRawParameterValue("lowQ")->load(), juce::Decibels::decibelsToGain(apvts->getRawParameterValue("lowGain")->load()));
    *lowMidPeakFilter.state = *Coefficients::makePeakFilter(sampleRate, eqFreq("lowMidFreq"), apvts->getRawParameterValue("lowMidQ")->load(), juce::Decibels::decibelsToGain(apvts->getRawParameterValue("lowMidGain")->load()));
    *highMidPeakFilter.state = *Coefficients::makePeakFilter(sampleRate, eqFreq("highMidFreq"), apvts->getRawParameterValue("highMidQ")->load(), juce::Decibels::decibelsToGain(apvts->getRawParameterValue("highMidGain")->load()));
    *highShelfFilter.state = *Coefficients::makeHighShelf(sampleRate, eqFreq("highFreq"), apvts->getRawParameterValue("highQ")->load(), juce::Decibels::decibelsToGain(apvts->getRawParameterValue("highGain")->load()));
    
    reverbParams.roomSize = *apvts->getRawParameterValue("roomSize");
    reverbParams.damping = *apvts->getRawParameterValue("damping");
    reverbParams.width = *apvts->getRawParameterValue("width");
    reverbParams.wetLevel = *apvts->getRawParameterValue("wet");
    reverbParams.dryLevel = *apvts->getRawParameterValue("dry");
    reverbParams.freezeMode = *apvts->getRawParameterValue("freeze");

    reverb.setParameters(reverbParams);

    comp.setThreshold(*apvts->getRawParameterValue("threshold"));
    comp.setRatio(*apvts->getRawParameterValue("ratio"));
    comp.setAttack(*apvts->getRawParameterValue("attack"));
    comp.setRelease(*apvts->getRawParameterValue("release"));

    chorus.setRate(*apvts->getRawParameterValue("lforate"));
    chorus.setDepth(*apvts->getRawParameterValue("lfodepth"));
    chorus.setCentreDelay(*apvts->getRawParameterValue("centerdelay"));
    chorus.setFeedback(*apvts->getRawParameterValue("chorfeedback"));
    chorus.setMix(*apvts->getRawParameterValue("chormix"));

    saturator.get<0>().setGainLinear(*apvts->getRawParameterValue("preGain"));
    saturator.get<2>().setGainLinear(*apvts->getRawParameterValue("postGain"));

    isPrepared = true;
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

void SuperAwesomeVocalChainAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    if (!isPrepared) {
        return;
    }
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto numSamples = buffer.getNumSamples();
    auto nCh = juce::jmin (totalNumInputChannels, totalNumOutputChannels);

    const float inputDb = apvts->getRawParameterValue ("inputGain")->load();
    const float outputDb = apvts->getRawParameterValue ("outputGain")->load();
    const float mixWet =
        juce::jlimit (0.0f, 1.0f, apvts->getRawParameterValue ("outputDryWet")->load());
    const bool bypassAllFx = apvts->getRawParameterValue ("allFxBypass")->load() > (0.5f - 1.0e-6f);
    const auto isBypassed = [this] (const char* paramID)
    {
        return apvts->getRawParameterValue (paramID)->load() > (0.5f - 1.0e-6f);
    };

    buffer.applyGain (juce::Decibels::decibelsToGain (inputDb));

    // Post-trim input peak (signal entering DSP chain — reflects user's input gain).
    {
        float peak = 0.0f;
        for (int ch = 0; ch < nCh; ++ch)
            peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, numSamples));
        const float prev = meterInputPeak.load (std::memory_order_relaxed);
        meterInputPeak.store (juce::jmax (peak, prev * 0.96f), std::memory_order_relaxed);
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    auto captureOutputPeak = [this, &buffer, nCh, numSamples]
    {
        float peak = 0.0f;
        for (int ch = 0; ch < nCh; ++ch)
            peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, numSamples));
        const float prev = meterOutputPeak.load (std::memory_order_relaxed);
        meterOutputPeak.store (juce::jmax (peak, prev * 0.96f), std::memory_order_relaxed);
    };

    if (bypassAllFx)
    {
        buffer.applyGain (juce::Decibels::decibelsToGain (outputDb));
        captureOutputPeak();
        return;
    }

    for (int ch = 0; ch < nCh; ++ch)
        dryWetBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    auto sampleRate = getSampleRate();

    if (eqNeedsUpdate.exchange(false)) {
        const auto eqFreq = [this, sampleRate] (const char* paramID)
        {
            return safeFilterFrequency (apvts->getRawParameterValue (paramID)->load(), sampleRate);
        };

        *lowShelfFilter.state = *Coefficients::makeLowShelf(sampleRate, eqFreq("lowFreq"), apvts->getRawParameterValue("lowQ")->load(), juce::Decibels::decibelsToGain(apvts->getRawParameterValue("lowGain")->load()));
        *lowMidPeakFilter.state = *Coefficients::makePeakFilter(sampleRate, eqFreq("lowMidFreq"), apvts->getRawParameterValue("lowMidQ")->load(), juce::Decibels::decibelsToGain(apvts->getRawParameterValue("lowMidGain")->load()));
        *highMidPeakFilter.state = *Coefficients::makePeakFilter(sampleRate, eqFreq("highMidFreq"), apvts->getRawParameterValue("highMidQ")->load(), juce::Decibels::decibelsToGain(apvts->getRawParameterValue("highMidGain")->load()));
        *highShelfFilter.state = *Coefficients::makeHighShelf(sampleRate, eqFreq("highFreq"), apvts->getRawParameterValue("highQ")->load(), juce::Decibels::decibelsToGain(apvts->getRawParameterValue("highGain")->load()));
    }
    

    // Update compressor parameters
    if (compNeedsUpdate.exchange(false)) {
        comp.setThreshold(*apvts->getRawParameterValue("threshold"));
        comp.setRatio(*apvts->getRawParameterValue("ratio"));
        comp.setAttack(*apvts->getRawParameterValue("attack"));
        comp.setRelease(*apvts->getRawParameterValue("release"));
    }
    


    // Update reverb parameters
    if (revNeedsUpdate.exchange(false)) {
        reverbParams.roomSize = *apvts->getRawParameterValue("roomSize");
        reverbParams.damping = *apvts->getRawParameterValue("damping");
        reverbParams.width = *apvts->getRawParameterValue("width");
        reverbParams.wetLevel = *apvts->getRawParameterValue("wet");
        reverbParams.dryLevel = *apvts->getRawParameterValue("dry");
        reverbParams.freezeMode = *apvts->getRawParameterValue("freeze");

        reverb.setParameters(reverbParams);
    }

    //Update chorus parameters
    if (chorNeedsUpdate.exchange(false)) {
        chorus.setRate(*apvts->getRawParameterValue("lforate"));
        chorus.setDepth(*apvts->getRawParameterValue("lfodepth"));
        chorus.setCentreDelay(*apvts->getRawParameterValue("centerdelay"));
        chorus.setFeedback(*apvts->getRawParameterValue("chorfeedback"));
        chorus.setMix(*apvts->getRawParameterValue("chormix"));
    }
    
    //Update saturator parameters
    if (satNeedsUpdate.exchange(false)) {
        saturator.get<0>().setGainLinear(*apvts->getRawParameterValue("preGain"));
        saturator.get<2>().setGainLinear(*apvts->getRawParameterValue("postGain"));
        currentSatMode.store ((int) *apvts->getRawParameterValue("satType"),
                              std::memory_order_relaxed);
    }

    const int chainIndex = static_cast<int> (
        std::lround ((double) apvts->getRawParameterValue ("fxChainOrder")->load()));
    std::array<uint8_t, 5> fxOrder {};
    permutationIndexToOrder (chainIndex, fxOrder);

    for (int step = 0; step < 5; ++step)
    {
        const auto fid = static_cast<FxEffectId> (fxOrder[(size_t) step]);
        switch (fid)
        {
            case FxEffectId::Eq:
                if (! isBypassed ("eqBypass"))
                {
                    lowShelfFilter.process (context);
                    lowMidPeakFilter.process (context);
                    highMidPeakFilter.process (context);
                    highShelfFilter.process (context);
                }
                break;
            case FxEffectId::Comp:
                if (! isBypassed ("compBypass"))
                    comp.process (context);
                break;
            case FxEffectId::Saturator:
                if (! isBypassed ("satBypass"))
                    saturator.process (context);
                break;
            case FxEffectId::Reverb:
                if (! isBypassed ("reverbBypass"))
                    reverb.process (context);
                break;
            case FxEffectId::Chorus:
                if (! isBypassed ("chorusBypass"))
                    chorus.process (context);
                break;
            default:
                break;
        }
    }

    const float dryAmt = 1.0f - mixWet;
    const float wetAmt = mixWet;

    if (wetAmt <= 1.0e-6f)
    {
        for (int ch = 0; ch < nCh; ++ch)
            buffer.copyFrom (ch, 0, dryWetBuffer, ch, 0, numSamples);
    }
    else if (dryAmt > 1.0e-6f)
    {
        for (int ch = 0; ch < nCh; ++ch)
        {
            auto w = buffer.getWritePointer(ch);
            auto d = dryWetBuffer.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
                w[i] = dryAmt * d[i] + wetAmt * w[i];
        }
    }

    buffer.applyGain (juce::Decibels::decibelsToGain (outputDb));
    captureOutputPeak();
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
    juce::ValueTree root ("SAFCState");
    root.setProperty ("lastPresetName", lastPresetName, nullptr);
    root.setProperty ("lastPresetSource", lastPresetSource, nullptr);
    root.appendChild (apvts->copyState(), nullptr);

    juce::ValueTree mappingsVT ("MacroMappings");
    if (macroController != nullptr)
    {
        for (const auto& m : macroController->getMappings())
        {
            juce::ValueTree mvt ("Mapping");
            mvt.setProperty ("targetParamID", m.targetParamID, nullptr);
            mvt.setProperty ("minValue",      m.minValue,      nullptr);
            mvt.setProperty ("maxValue",      m.maxValue,      nullptr);
            mvt.setProperty ("curve",         m.curve,         nullptr);
            mvt.setProperty ("inverted",      m.inverted,      nullptr);
            mappingsVT.appendChild (mvt, nullptr);
        }
    }
    root.appendChild (mappingsVT, nullptr);

    if (auto xml = root.createXml())
        copyXmlToBinary (*xml, destData);
}

void SuperAwesomeVocalChainAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;

    auto root = juce::ValueTree::fromXml (*xml);
    if (! root.isValid()) return;

    if (root.hasProperty ("lastPresetName"))
        lastPresetName = root.getProperty ("lastPresetName").toString();

    if (root.hasProperty ("lastPresetSource"))
        lastPresetSource = root.getProperty ("lastPresetSource").toString();

    auto apvtsState = root.getChildWithName (apvts->state.getType());
    if (apvtsState.isValid())
        apvts->replaceState (apvtsState);

    if (macroController == nullptr) return;

    auto mappingsVT = root.getChildWithName ("MacroMappings");
    if (! mappingsVT.isValid()) return;

    std::vector<MacroMapping> mappings;
    for (auto child : mappingsVT)
    {
        if (! child.hasType ("Mapping")) continue;
        MacroMapping m;
        m.targetParamID = child.getProperty ("targetParamID").toString();
        m.minValue      = (float) child.getProperty ("minValue");
        m.maxValue      = (float) child.getProperty ("maxValue");
        m.curve         = (float) child.getProperty ("curve");
        m.inverted      = (bool)  child.getProperty ("inverted");
        mappings.push_back (m);
    }
    macroController->setMappings (std::move (mappings));
}

juce::AudioProcessorValueTreeState::ParameterLayout SuperAwesomeVocalChainAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Create parameters for the EQ effect
    // Low Band (Low Shelf)
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "lowFreq", 1 }, "Low Freq", 20.0f, 500.0f, 200.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "lowGain", 1 }, "Low Gain", -24.0f, 24.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "lowQ", 1 }, "Low Q", 0.1f, 10.0f, 0.707f));

    // Low-Mid Band (Peak)
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "lowMidFreq", 1 }, "Low-Mid Freq", 200.0f, 2000.0f, 500.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "lowMidGain", 1 }, "Low-Mid Gain", -24.0f, 24.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "lowMidQ", 1 }, "Low-Mid Q", 0.1f, 10.0f, 0.707f));

    // High-Mid Band (Peak)
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "highMidFreq", 1 }, "High-Mid Freq", 2000.0f, 8000.0f, 3000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "highMidGain", 1 }, "High-Mid Gain", -24.0f, 24.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "highMidQ", 1 }, "High-Mid Q", 0.1f, 10.0f, 0.707f));

    // High Band (High Shelf)
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "highFreq", 1 }, "High Freq", 5000.0f, 20000.0f, 10000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "highGain", 1 }, "High Gain", -24.0f, 24.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "highQ", 1 }, "High Q", 0.1f, 10.0f, 0.707f));

    // Create parameters for the reverb effect
    layout.add(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "roomSize", 1 }, "Room Size", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "damping", 1 }, "Damping", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "width", 1 }, "Width", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "wet", 1 }, "Wet Level", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "dry", 1 }, "Dry Level", 0.0f, 1.0f, 0.55f));
    layout.add(std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "freeze", 1 }, "Freeze", false));

    // Create parameter for compressor effect
    layout.add(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "threshold", 1 }, "Threshold", -60.0f, 0.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "ratio", 1 }, "Ratio", 1.0f, 10.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "attack", 1 }, "Attack", 2.0f, 200.0f, 2.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "release", 1 }, "Release", 30.0f, 1000.0f, 30.0f));

    //Create parameters for Chorus effect
    layout.add(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "lforate", 1 }, "Rate",   0.0f, 2.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "lfodepth", 1 }, "Depth", 0.0f, 1.0f, 0.25f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "centerdelay", 1 }, "Center Delay", 1.0f, 25.0f, 10.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "chorfeedback", 1 }, "Feedback", -1.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "chormix", 1 }, "Mix", 0.0f, 1.0f, 0.0f));

    //Create parameters for Saturator
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "preGain", 1 }, "Pre-Gain", 0.0f, 5.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "postGain", 1 }, "Post-Gain", 0.0f, 5.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "satType", 1 }, "Saturation Type",
        juce::StringArray { "Cubic", "Soft", "Tape", "Tube", "Hard" }, 1));

    // Macro knob (0..1), drives mapped parameters via MacroController.
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "macro", 1 }, "Macro", 0.0f, 1.0f, 0.5f));

    /** 0 … 119 = ordering of Eq,Comp,Sat,Chorus,Reverb. Default 0 = Eq→Comp→Sat→Chorus→Reverb. */
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "fxChainOrder", 1 }, "FX Chain Order", 0, 119, 0));

    // Bypass switches for each module
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "eqBypass", 1 },      "EQ Bypass",      false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "compBypass", 1 },    "Comp Bypass",    false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "reverbBypass", 1 },  "Reverb Bypass",  false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "chorusBypass", 1 },  "Chorus Bypass",  false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "satBypass", 1 },     "Sat Bypass",     false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "inputGain", 1 }, "Input Gain", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outputGain", 1 }, "Output Gain", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "outputDryWet", 1 }, "Output Dry/Wet", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "allFxBypass", 1 }, "Bypass All Effects", false));

    return layout;
}
//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SuperAwesomeVocalChainAudioProcessor();
}
