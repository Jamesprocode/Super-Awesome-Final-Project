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
    macroKnob.setDoubleClickReturnValue(true, 0.5); // optional

    macroPage.addAndMakeVisible(macroKnob);

    macroKnobAttachment = std::make_unique<
        juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, "macro", macroKnob);

    // PAGE 3: detailed page

    // EQ parameters
    auto& apvts = audioProcessor.apvts;

    // Helper to place knobs
    auto setupKnob = [&](juce::Slider& s)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        detailPage.addAndMakeVisible(s);
    };

    // Helper to place knob labels
    auto setupLabel = [&](juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        detailPage.addAndMakeVisible(label);
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

    // Compressor parameters
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

    // Saturator parameters
    setupLabel(saturateLabel, "Saturation");
    setupKnob(preGainSlider);
    setupKnob(postGainSlider);
    setupLabel(preGainLabel, "Pre-Gain");
    setupLabel(postGainLabel, "Post-Gain");

    preGainAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "preGain", preGainSlider);
    postGainAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "postGain", postGainSlider);

    // Inspector inspects `content`, not the editor (avoids self‑introspection)
    inspector = std::make_unique<melatonin::Inspector>(content);
    addAndMakeVisible(*inspector);

    startTimer(50);
}

SuperAwesomeVocalChainAudioProcessorEditor::~SuperAwesomeVocalChainAudioProcessorEditor()
{
}

//==============================================================================
void SuperAwesomeVocalChainAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void SuperAwesomeVocalChainAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // --- Inspector on the right ---
    const int inspectorWidth = 350;
    auto inspectorArea = area.removeFromRight(inspectorWidth);

    if (inspector)
        inspector->setBounds(inspectorArea);

    // --- Content on the left ---
    content.setBounds(area);
    auto contentBounds = content.getLocalBounds();

    // --- Tab bar ---
    const int tabHeight = 50;
    auto tabBar = contentBounds.removeFromBottom(tabHeight);

    // Tab widths must use tabBar.getWidth(), not contentBounds.getWidth()
    int tabWidth = tabBar.getWidth() / 3;

    macroTab.setBounds(tabBar.removeFromLeft(tabWidth));
    mapTab.setBounds(tabBar.removeFromLeft(tabWidth));
    detailTab.setBounds(tabBar);

    // --- Pages (all same size, stacked) ---
    macroPage.setBounds(contentBounds);
    mapPage.setBounds(contentBounds);
    detailPage.setBounds(contentBounds);

    // PAGE 1: macro page
    auto macroPageArea = macroPage.getLocalBounds().reduced(40);

    if (macroPageArea.getWidth() > 0 && macroPageArea.getHeight() > 0)
    {
        int knobSize = juce::jmin(macroPageArea.getWidth(), macroPageArea.getHeight()) * 0.6;

        auto knobArea = juce::Rectangle<int>(knobSize, knobSize)
                            .withCentre(macroPageArea.getCentre());

        macroKnob.setBounds(knobArea);
    }

    // PAGE 3: detailed page
    auto detailPageArea = detailPage.getLocalBounds().reduced(20);

    const int knobSize = 70;
    const int spacing  = 20;
    const int labelHeight = 20;

    eqLabel.setBounds(detailPageArea.removeFromTop(labelHeight));

    auto eqArea = detailPageArea;   // save full EQ region BEFORE carving columns

    // 4 columns (Low, LowMid, HighMid, High)
    auto columnWidth = eqArea.getWidth() / 4;

    auto lowCol     = eqArea.removeFromLeft(columnWidth);
    auto lowMidCol  = eqArea.removeFromLeft(columnWidth);
    auto highMidCol = eqArea.removeFromLeft(columnWidth);
    auto highCol    = eqArea;

    auto layoutColumn = [&](auto& freq, auto& freqLabel,
                        auto& gain, auto& gainLabel,
                        auto& q,    auto& qLabel,
                        juce::Rectangle<int> col)
    {

        auto centerX = col.getCentreX();
        int y = col.getY();

        // Freq knob + label
        freq.setBounds(centerX - knobSize/2, y, knobSize, knobSize);
        freqLabel.setBounds(centerX - knobSize/2, y + knobSize, knobSize, labelHeight);
        y += knobSize + labelHeight + spacing;

        // Gain knob + label
        gain.setBounds(centerX - knobSize/2, y, knobSize, knobSize);
        gainLabel.setBounds(centerX - knobSize/2, y + knobSize, knobSize, labelHeight);
        y += knobSize + labelHeight + spacing;

        // Q knob + label
        q.setBounds(centerX - knobSize/2, y, knobSize, knobSize);
        qLabel.setBounds(centerX - knobSize/2, y + knobSize, knobSize, labelHeight);
    };

    layoutColumn(lowFreqSlider, lowFreqLabel,
             lowGainSlider, lowGainLabel,
             lowQSlider,    lowQLabel,
             lowCol);

    layoutColumn(lowMidFreqSlider, lowMidFreqLabel,
                 lowMidGainSlider, lowMidGainLabel,
                 lowMidQSlider,    lowMidQLabel,
                 lowMidCol);

    layoutColumn(highMidFreqSlider, highMidFreqLabel,
                 highMidGainSlider, highMidGainLabel,
                 highMidQSlider,    highMidQLabel,
                 highMidCol);

    layoutColumn(highFreqSlider, highFreqLabel,
                 highGainSlider, highGainLabel,
                 highQSlider,    highQLabel,
                 highCol);

    // Compressor
    auto compArea = detailPageArea;

    // Move compArea down below the EQ rows
    compArea.removeFromTop(detailPageArea.getHeight() / 2);

    compLabel.setBounds(compArea.removeFromTop(labelHeight));

    auto row = compArea.withHeight(knobSize);

    int x = row.getX();
    int y = row.getY();

    thresholdSlider.setBounds(x, y, knobSize, knobSize);
    thresholdLabel.setBounds(x, y + knobSize, knobSize,labelHeight);
    x += knobSize + spacing;

    ratioSlider.setBounds(x, y, knobSize, knobSize);
    ratioLabel.setBounds(x, y + knobSize, knobSize,labelHeight);
    x += knobSize + spacing;

    attackSlider.setBounds(x, y, knobSize, knobSize);
    attackLabel.setBounds(x, y + knobSize, knobSize, labelHeight);
    x += knobSize + spacing;

    releaseSlider.setBounds(x, y, knobSize, knobSize);
    releaseLabel.setBounds(x, y + knobSize, knobSize, labelHeight);

    // Saturator
    auto saturateArea = compArea;

    // Move compArea down below the EQ rows
    saturateArea.removeFromTop(compArea.getHeight() / 2);

    saturateLabel.setBounds(saturateArea.removeFromTop(labelHeight));

    auto saturateRow = saturateArea.withHeight(knobSize);

    int saturatex = saturateRow.getX();
    int saturatey = saturateRow.getY();

    preGainSlider.setBounds(saturatex, saturatey, knobSize, knobSize);
    preGainLabel.setBounds(saturatex, saturatey + knobSize, knobSize, labelHeight);
    saturatex += knobSize + spacing;

    postGainSlider.setBounds(saturatex, saturatey, knobSize, knobSize);
    postGainLabel.setBounds(saturatex, saturatey + knobSize, knobSize, labelHeight);
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

void SuperAwesomeVocalChainAudioProcessorEditor::updateVisibility()
{
    /*auto* lowFreq   = audioProcessor.apvts.getRawParameterValue("lowFreq");
    auto* threshold = audioProcessor.apvts.getRawParameterValue("threshold");
    auto* preGain   = audioProcessor.apvts.getRawParameterValue("preGain");

    if (inspector)
    {
        if (lowFreq)
            inspector->getProperties().set("Low Freq", lowFreq->load());

        if (threshold)
            inspector->getProperties().set("Threshold", threshold->load());

        if (preGain)
            inspector->getProperties().set("Pre-Gain", preGain->load());
    }*/
}