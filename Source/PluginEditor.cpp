#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SuperAwesomeVocalChainAudioProcessorEditor::SuperAwesomeVocalChainAudioProcessorEditor
    (SuperAwesomeVocalChainAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (1000, 800);

    // Content is the root for all your UI
    addAndMakeVisible(content);

    // Add pages to content
    content.addAndMakeVisible(macroPage);
    content.addAndMakeVisible(mapPage);
    content.addAndMakeVisible(detailPage);

    // Add tab buttons
    content.addAndMakeVisible(macroTab);
    content.addAndMakeVisible(mapTab);
    content.addAndMakeVisible(detailTab);

    // Tab callbacks
    macroTab.onClick = [this] { showPage(0); };
    mapTab.onClick  = [this] { showPage(1); };
    detailTab.onClick   = [this] { showPage(2); };

    // Start on macro page
    showPage(0);

    // PAGE 1: macro page
    macroKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    macroKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 30);
    macroKnob.setRange(0.0, 1.0, 0.001);
    macroKnob.setDoubleClickReturnValue(true, 0.5);

    macroPage.addAndMakeVisible(macroKnob);

    macroKnobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "macro", macroKnob);

    // PAGE 3: detailed page
    detailPage.addAndMakeVisible(detailViewport);
    detailViewport.setViewedComponent(&detailContent, false);
    detailViewport.setScrollBarsShown(true, false);

    // EQ parameters
    auto setupKnob = [&](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        detailContent.addAndMakeVisible(s);
    };

    auto setupLabel = [&](juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        detailContent.addAndMakeVisible(label);
    };

    setupLabel(eqLabel, "EQ");

    // Low band
    setupKnob(lowFreqSlider);
    setupKnob(lowGainSlider);
    setupKnob(lowQSlider);
    setupLabel(lowFreqLabel, "L-Freq");
    setupLabel(lowGainLabel, "L-Gain");
    setupLabel(lowQLabel,    "L-Q");

    lowFreqAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "lowFreq", lowFreqSlider);
    lowGainAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "lowGain", lowGainSlider);
    lowQAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "lowQ",    lowQSlider);

    // Low-mid band
    setupKnob(lowMidFreqSlider);
    setupKnob(lowMidGainSlider);
    setupKnob(lowMidQSlider);
    setupLabel(lowMidFreqLabel, "LM-Freq");
    setupLabel(lowMidGainLabel, "LM-Gain");
    setupLabel(lowMidQLabel,    "LM-Q");

    lowMidFreqAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "lowMidFreq", lowMidFreqSlider);
    lowMidGainAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "lowMidGain", lowMidGainSlider);
    lowMidQAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "lowMidQ",    lowMidQSlider);

    // High-mid band
    setupKnob(highMidFreqSlider);
    setupKnob(highMidGainSlider);
    setupKnob(highMidQSlider);
    setupLabel(highMidFreqLabel, "HM-Freq");
    setupLabel(highMidGainLabel, "HM-Gain");
    setupLabel(highMidQLabel,    "HM-Q");

    highMidFreqAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "highMidFreq", highMidFreqSlider);
    highMidGainAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "highMidGain", highMidGainSlider);
    highMidQAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "highMidQ",    highMidQSlider);

    // High band
    setupKnob(highFreqSlider);
    setupKnob(highGainSlider);
    setupKnob(highQSlider);
    setupLabel(highFreqLabel, "H-Freq");
    setupLabel(highGainLabel, "H-Gain");
    setupLabel(highQLabel,    "H-Q");

    highFreqAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "highFreq", highFreqSlider);
    highGainAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "highGain", highGainSlider);
    highQAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "highQ",    highQSlider);

    // Compressor
    setupLabel(compLabel, "Compressor");
    setupKnob(thresholdSlider);
    setupKnob(ratioSlider);
    setupKnob(attackSlider);
    setupKnob(releaseSlider);
    setupLabel(thresholdLabel, "Threshold");
    setupLabel(ratioLabel, "Ratio");
    setupLabel(attackLabel, "Attack");
    setupLabel(releaseLabel, "Release");

    thresholdAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "threshold", thresholdSlider);
    ratioAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "ratio",    ratioSlider);
    attackAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "attack", attackSlider);
    releaseAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "release",    releaseSlider);

    // Saturator
    setupLabel(saturateLabel, "Saturation");
    setupKnob(preGainSlider);
    setupKnob(postGainSlider);
    setupLabel(preGainLabel, "Pre-Gain");
    setupLabel(postGainLabel, "Post-Gain");

    preGainAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "preGain", preGainSlider);
    postGainAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "postGain", postGainSlider);

    // Chorus
    setupLabel(chorusLabel, "Chorus");
    setupKnob(lfoRateSlider);
    setupKnob(lfoDepthSlider);
    setupKnob(centerDelaySlider);
    setupKnob(chorusFeedbackSlider);
    setupKnob(chorusMixSlider);
    setupLabel(lfoRateLabel, "LFO Rate");
    setupLabel(lfoDepthLabel, "LFO Depth");
    setupLabel(centerDelayLabel, "Center Delay");
    setupLabel(chorusFeedbackLabel, "Feedback");
    setupLabel(chorusMixLabel, "Mix");

    lfoRateAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "lforate", lfoRateSlider);
    lfoDepthAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "lfodepth", lfoDepthSlider);
    centerDelayAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "centerdelay", centerDelaySlider);
    chorusFeedbackAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "chorfeedback", chorusFeedbackSlider);
    chorusMixAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "chormix", chorusMixSlider);

    // Reverb
    setupLabel(reverbLabel, "Reverb");
    setupKnob(roomSizeSlider);
    setupKnob(dampingSlider);
    setupKnob(reverbWidthSlider);
    setupKnob(reverbWetSlider);
    setupKnob(reverbDrySlider);
    setupKnob(freezeModeSlider);
    setupLabel(roomSizeLabel, "Room Size");
    setupLabel(dampingLabel, "Damping");
    setupLabel(reverbWidthLabel, "Reverb Width");
    setupLabel(reverbWetLabel, "Reverb Wet");
    setupLabel(reverbDryLabel, "Reverb Dry");
    setupLabel(freezeModeLabel, "Freeze Mode");

    roomSizeAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "roomSize", roomSizeSlider);
    dampingAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "damping", dampingSlider);
    reverbWidthAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "width", reverbWidthSlider);
    reverbWetAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "wet", reverbWetSlider);
    reverbDryAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "dry", reverbDrySlider);
    freezeModeAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "freezeMode", freezeModeSlider);

    inspector = std::make_unique<melatonin::Inspector>(content);
    addAndMakeVisible(*inspector);

    startTimer(50);
}

SuperAwesomeVocalChainAudioProcessorEditor::~SuperAwesomeVocalChainAudioProcessorEditor() {}

void SuperAwesomeVocalChainAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void SuperAwesomeVocalChainAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // --- Inspector ---
    const int inspectorWidth = 350;
    if (inspector)
        inspector->setBounds(area.removeFromRight(inspectorWidth));

    // --- Main Content ---
    content.setBounds(area);
    auto contentBounds = content.getLocalBounds();

    // --- Tab bar ---
    const int tabHeight = 50;
    auto tabBar = contentBounds.removeFromBottom(tabHeight);
    int tabWidth = tabBar.getWidth() / 3;

    macroTab.setBounds(tabBar.removeFromLeft(tabWidth));
    mapTab.setBounds(tabBar.removeFromLeft(tabWidth));
    detailTab.setBounds(tabBar);

    // --- Pages ---
    macroPage.setBounds(contentBounds);
    mapPage.setBounds(contentBounds);
    detailPage.setBounds(contentBounds);
    detailViewport.setBounds(detailPage.getLocalBounds());

    // --- Macro Layout ---
    auto macroPageArea = macroPage.getLocalBounds().reduced(40);
    int mKnobSize = juce::jmin(macroPageArea.getWidth(), macroPageArea.getHeight()) * 0.6;
    macroKnob.setBounds(juce::Rectangle<int>(mKnobSize, mKnobSize).withCentre(macroPageArea.getCentre()));

    // --- Detailed Layout (Scrolling Content) ---
    const int sectionHeight = 320;
    const int knobSize = 70;
    const int spacing  = 20;
    const int labelHeight = 20;

    detailContent.setSize(detailViewport.getWidth(), 1400); // Fixed total height
    auto fullArea = detailContent.getLocalBounds().reduced(20);

    // Reusable lambda for EQ columns
    auto layoutColumn = [&](auto& freq, auto& freqLabel, auto& gain, auto& gainLabel, auto& q, auto& qLabel, juce::Rectangle<int> col)
    {
        auto centerX = col.getCentreX();
        int y = col.getY();
        freq.setBounds(centerX - knobSize/2, y, knobSize, knobSize);
        freqLabel.setBounds(centerX - knobSize/2, y + knobSize, knobSize, labelHeight);
        y += knobSize + labelHeight + spacing;
        gain.setBounds(centerX - knobSize/2, y, knobSize, knobSize);
        gainLabel.setBounds(centerX - knobSize/2, y + knobSize, knobSize, labelHeight);
        y += knobSize + labelHeight + spacing;
        q.setBounds(centerX - knobSize/2, y, knobSize, knobSize);
        qLabel.setBounds(centerX - knobSize/2, y + knobSize, knobSize, labelHeight);
    };

    // 1. EQ Section
    auto eqArea = fullArea.removeFromTop(sectionHeight);
    eqLabel.setBounds(eqArea.removeFromTop(labelHeight));
    auto colWidth = eqArea.getWidth() / 4;
    layoutColumn(lowFreqSlider, lowFreqLabel, lowGainSlider, lowGainLabel, lowQSlider, lowQLabel, eqArea.removeFromLeft(colWidth));
    layoutColumn(lowMidFreqSlider, lowMidFreqLabel, lowMidGainSlider, lowMidGainLabel, lowMidQSlider, lowMidQLabel, eqArea.removeFromLeft(colWidth));
    layoutColumn(highMidFreqSlider, highMidFreqLabel, highMidGainSlider, highMidGainLabel, highMidQSlider, highMidQLabel, eqArea.removeFromLeft(colWidth));
    layoutColumn(highFreqSlider, highFreqLabel, highGainSlider, highGainLabel, highQSlider, highQLabel, eqArea);

    // 2. Compressor Section
    auto compArea = fullArea.removeFromTop(150);
    compLabel.setBounds(compArea.removeFromTop(labelHeight));
    int cx = compArea.getX();
    int cy = compArea.getY();
    thresholdSlider.setBounds(cx, cy, knobSize, knobSize);
    thresholdLabel.setBounds(cx, cy + knobSize, knobSize, labelHeight);
    cx += knobSize + spacing;
    ratioSlider.setBounds(cx, cy, knobSize, knobSize);
    ratioLabel.setBounds(cx, cy + knobSize, knobSize, labelHeight);
    cx += knobSize + spacing;
    attackSlider.setBounds(cx, cy, knobSize, knobSize);
    attackLabel.setBounds(cx, cy + knobSize, knobSize, labelHeight);
    cx += knobSize + spacing;
    releaseSlider.setBounds(cx, cy, knobSize, knobSize);
    releaseLabel.setBounds(cx, cy + knobSize, knobSize, labelHeight);

    // 3. Saturator Section
    auto satArea = fullArea.removeFromTop(150);
    saturateLabel.setBounds(satArea.removeFromTop(labelHeight));
    int sx = satArea.getX();
    int sy = satArea.getY();
    preGainSlider.setBounds(sx, sy, knobSize, knobSize);
    preGainLabel.setBounds(sx, sy + knobSize, knobSize, labelHeight);
    sx += knobSize + spacing;
    postGainSlider.setBounds(sx, sy, knobSize, knobSize);
    postGainLabel.setBounds(sx, sy + knobSize, knobSize, labelHeight);

    // 4. Chorus Section
    auto chorArea = fullArea.removeFromTop(150);
    chorusLabel.setBounds(chorArea.removeFromTop(labelHeight));
    int chx = chorArea.getX();
    int chy = chorArea.getY();
    lfoRateSlider.setBounds(chx, chy, knobSize, knobSize);
    lfoRateLabel.setBounds(chx, chy + knobSize, knobSize, labelHeight);
    chx += knobSize + spacing;
    lfoDepthSlider.setBounds(chx, chy, knobSize, knobSize);
    lfoDepthLabel.setBounds(chx, chy + knobSize, knobSize, labelHeight);
    chx += knobSize + spacing;
    centerDelaySlider.setBounds(chx, chy, knobSize, knobSize);
    centerDelayLabel.setBounds(chx, chy + knobSize, knobSize, labelHeight);
    chx += knobSize + spacing;
    chorusFeedbackSlider.setBounds(chx, chy, knobSize, knobSize);
    chorusFeedbackLabel.setBounds(chx, chy + knobSize, knobSize, labelHeight);
    chx += knobSize + spacing;
    chorusMixSlider.setBounds(chx, chy, knobSize, knobSize);
    chorusMixLabel.setBounds(chx, chy + knobSize, knobSize, labelHeight);

    // Reverb Section
    auto revArea = fullArea.removeFromTop(150);
    reverbLabel.setBounds(revArea.removeFromTop(labelHeight));
    int rvx = revArea.getX();
    int rvy = revArea.getY();
    roomSizeSlider.setBounds(rvx, rvy, knobSize, knobSize);
    roomSizeLabel.setBounds(rvx, rvy + knobSize, knobSize, labelHeight);
    rvx += knobSize + spacing;
    dampingSlider.setBounds(rvx, rvy, knobSize, knobSize);
    dampingLabel.setBounds(rvx, rvy + knobSize, knobSize, labelHeight);
    rvx += knobSize + spacing;
    reverbWidthSlider.setBounds(rvx, rvy, knobSize, knobSize);
    reverbWidthLabel.setBounds(rvx, rvy + knobSize, knobSize, labelHeight);
    rvx += knobSize + spacing;
    reverbWetSlider.setBounds(rvx, rvy, knobSize, knobSize);
    reverbWetLabel.setBounds(rvx, rvy + knobSize, knobSize, labelHeight);
    rvx += knobSize + spacing;
    reverbDrySlider.setBounds(rvx, rvy, knobSize, knobSize);
    reverbDryLabel.setBounds(rvx, rvy + knobSize, knobSize, labelHeight);
    rvx += knobSize + spacing;
    freezeModeSlider.setBounds(rvx, rvy, knobSize, knobSize);
    freezeModeLabel.setBounds(rvx, rvy + knobSize, knobSize, labelHeight);
}

void SuperAwesomeVocalChainAudioProcessorEditor::showPage(int index)
{
    macroPage.setVisible(index == 0);
    mapPage.setVisible(index == 1);
    detailPage.setVisible(index == 2);
}

void SuperAwesomeVocalChainAudioProcessorEditor::timerCallback()
{
    updateVisibility();
    stopTimer();
}

void SuperAwesomeVocalChainAudioProcessorEditor::updateVisibility() {}