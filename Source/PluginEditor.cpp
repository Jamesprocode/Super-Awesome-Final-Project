#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstddef>
#include <cstring>
#include <vector>

namespace
{

juce::File getUiPublicFolder()
{
    return juce::File{ __FILE__ }
        .getParentDirectory()
        .getParentDirectory()
        .getChildFile ("plugin")
        .getChildFile ("ui")
        .getChildFile ("public");
}

juce::String getMimeTypeForExtension (const juce::String& extLowerNoDot)
{
    if (extLowerNoDot == "htm")   return "text/html";
    if (extLowerNoDot == "html")  return "text/html";
    if (extLowerNoDot == "txt")   return "text/plain";
    if (extLowerNoDot == "css")   return "text/css";
    if (extLowerNoDot == "js")    return "text/javascript";
    if (extLowerNoDot == "json")  return "application/json";
    if (extLowerNoDot == "png")   return "image/png";
    if (extLowerNoDot == "jpg")  return "image/jpeg";
    if (extLowerNoDot == "jpeg")  return "image/jpeg";
    if (extLowerNoDot == "svg")  return "image/svg+xml";
    if (extLowerNoDot == "ico")  return "image/vnd.microsoft.icon";
    if (extLowerNoDot == "woff2")  return "font/woff2";

    return "application/octet-stream";
}

std::vector<std::byte> loadFileToByteVector (const juce::File& file)
{
    juce::MemoryBlock block;
    if (! file.loadFileAsData (block))
        return {};

    std::vector<std::byte> v ((size_t) block.getSize());
    if (block.getSize() > 0)
        std::memcpy (v.data(), block.getData(), (size_t) block.getSize());

    return v;
}

} // namespace

juce::WebBrowserComponent::Options SuperAwesomeVocalChainAudioProcessorEditor::createWebViewOptions (SuperAwesomeVocalChainAudioProcessorEditor& self)
{
    juce::WebBrowserComponent::Options o = juce::WebBrowserComponent::Options()
        .withNativeIntegrationEnabled (true)
        .withResourceProvider (
            [&self] (const juce::String& url) { return self.getResource (url); })
        .withOptionsFrom (self.macroSliderRelay);

   #if JUCE_WINDOWS
    o = o.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2()
            .withBackgroundColour (juce::Colours::white));
   #endif

    return o;
}

std::optional<SuperAwesomeVocalChainAudioProcessorEditor::Resource> SuperAwesomeVocalChainAudioProcessorEditor::getResource (const juce::String& url)
{
   #if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    const juce::String rel = url == "/" || url.isEmpty()
        ? "index.html"
        : juce::String{ url.fromFirstOccurrenceOf ("/", false, false) };

    if (rel.contains ("/../") || rel.startsWith ("..") || juce::File::isAbsolutePath (rel))
        return std::nullopt;

    const auto publicDir = getUiPublicFolder();
    const juce::File file = publicDir.getChildFile (rel);

    if (! file.isAChildOf (publicDir) || ! file.existsAsFile())
        return std::nullopt;

    juce::String ext = file.getFileExtension().toLowerCase();
    if (ext.isNotEmpty() && ext[0] == '.')
        ext = ext.fromFirstOccurrenceOf (".", false, false);

    auto bytes = loadFileToByteVector (file);
    if (file.getSize() > 0 && bytes.empty())
        return std::nullopt;

    return Resource { std::move (bytes), getMimeTypeForExtension (ext) };
   #else
    juce::ignoreUnused (url);
    return std::nullopt;
   #endif
}

//==============================================================================
SuperAwesomeVocalChainAudioProcessorEditor::SuperAwesomeVocalChainAudioProcessorEditor
    (SuperAwesomeVocalChainAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), webView (createWebViewOptions (*this))
{
    setSize (1000, 600);

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

    // PAGE 1: macro control is implemented in the WebView (React) — see WebSliderParameterAttachment

    // PAGE 2: mapping page (old single-dropdown version)
    // paramListBox.addItem("Low Frequency", 1);
    // paramListBox.addItem("Low Gain", 2);
    // paramListBox.addItem("Low Q", 3);
    // paramListBox.addItem("Low-Mid Frequency", 4);
    // paramListBox.addItem("Low-Mid Gain", 5);
    // paramListBox.addItem("Low-Mid Q", 6);
    // paramListBox.addItem("High-Mid Frequency", 7);
    // paramListBox.addItem("High-Mid Gain", 8);
    // paramListBox.addItem("High-Mid Q", 9);
    // paramListBox.addItem("High Frequency", 10);
    // paramListBox.addItem("High Gain", 11);
    // paramListBox.addItem("High Q", 12);
    // paramListBox.setSelectedId(1, juce::dontSendNotification);
    // mapPage.addAndMakeVisible(paramListBox);
    // curveTypeBox.addItem("Linear", 1);
    // curveTypeBox.addItem("Logarithmic", 2);
    // curveTypeBox.setSelectedId(1, juce::dontSendNotification);
    // mapPage.addAndMakeVisible(curveTypeBox);

    // New: 5 blocks, one per DSP section
    setupMappingBlock(0, "EQ", {
        {"lowFreq","Low Freq"}, {"lowGain","Low Gain"}, {"lowQ","Low Q"},
        {"lowMidFreq","LM Freq"}, {"lowMidGain","LM Gain"}, {"lowMidQ","LM Q"},
        {"highMidFreq","HM Freq"}, {"highMidGain","HM Gain"}, {"highMidQ","HM Q"},
        {"highFreq","High Freq"}, {"highGain","High Gain"}, {"highQ","High Q"}
    });
    setupMappingBlock(1, "Compressor", {
        {"threshold","Threshold"}, {"ratio","Ratio"}, {"attack","Attack"}, {"release","Release"}
    });
    setupMappingBlock(2, "Saturator", {
        {"preGain","Pre-Gain"}, {"postGain","Post-Gain"}
    });
    setupMappingBlock(3, "Chorus", {
        {"lforate","LFO Rate"}, {"lfodepth","LFO Depth"}, {"centerdelay","Center Delay"},
        {"chorfeedback","Feedback"}, {"chormix","Mix"}
    });
    setupMappingBlock(4, "Reverb", {
        {"roomSize","Room Size"}, {"damping","Damping"}, {"width","Width"},
        {"wet","Wet"}, {"dry","Dry"}, {"freeze","Freeze"}
    });
    refreshMappingHighlights();

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
    detailContent.addAndMakeVisible(eqBypassButton);
    eqBypassAttach = std::make_unique<ButtonAttachment>(*audioProcessor.apvts, "eqBypass", eqBypassButton);

    // Low band
    setupKnob(lowFreqSlider);
    setupKnob(lowGainSlider);
    setupKnob(lowQSlider);
    setupLabel(lowFreqLabel, "L-Freq");
    setupLabel(lowGainLabel, "L-Gain");
    setupLabel(lowQLabel,    "L-Q");

    lowFreqAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "lowFreq", lowFreqSlider);
    lowGainAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "lowGain", lowGainSlider);
    lowQAttach    = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "lowQ",    lowQSlider);

    // Low-mid band
    setupKnob(lowMidFreqSlider);
    setupKnob(lowMidGainSlider);
    setupKnob(lowMidQSlider);
    setupLabel(lowMidFreqLabel, "LM-Freq");
    setupLabel(lowMidGainLabel, "LM-Gain");
    setupLabel(lowMidQLabel,    "LM-Q");

    lowMidFreqAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "lowMidFreq", lowMidFreqSlider);
    lowMidGainAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "lowMidGain", lowMidGainSlider);
    lowMidQAttach    = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "lowMidQ",    lowMidQSlider);

    // High-mid band
    setupKnob(highMidFreqSlider);
    setupKnob(highMidGainSlider);
    setupKnob(highMidQSlider);
    setupLabel(highMidFreqLabel, "HM-Freq");
    setupLabel(highMidGainLabel, "HM-Gain");
    setupLabel(highMidQLabel,    "HM-Q");

    highMidFreqAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "highMidFreq", highMidFreqSlider);
    highMidGainAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "highMidGain", highMidGainSlider);
    highMidQAttach    = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "highMidQ",    highMidQSlider);

    // High band
    setupKnob(highFreqSlider);
    setupKnob(highGainSlider);
    setupKnob(highQSlider);
    setupLabel(highFreqLabel, "H-Freq");
    setupLabel(highGainLabel, "H-Gain");
    setupLabel(highQLabel,    "H-Q");

    highFreqAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "highFreq", highFreqSlider);
    highGainAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "highGain", highGainSlider);
    highQAttach    = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "highQ",    highQSlider);

    // Compressor
    setupLabel(compLabel, "Compressor");
    detailContent.addAndMakeVisible(compBypassButton);
    compBypassAttach = std::make_unique<ButtonAttachment>(*audioProcessor.apvts, "compBypass", compBypassButton);
    setupKnob(thresholdSlider);
    setupKnob(ratioSlider);
    setupKnob(attackSlider);
    setupKnob(releaseSlider);
    setupLabel(thresholdLabel, "Threshold");
    setupLabel(ratioLabel, "Ratio");
    setupLabel(attackLabel, "Attack");
    setupLabel(releaseLabel, "Release");

    thresholdAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "threshold", thresholdSlider);
    ratioAttach    = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "ratio",    ratioSlider);
    attackAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "attack", attackSlider);
    releaseAttach    = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "release",    releaseSlider);

    // Saturator
    setupLabel(saturateLabel, "Saturation");
    detailContent.addAndMakeVisible(satBypassButton);
    satBypassAttach = std::make_unique<ButtonAttachment>(*audioProcessor.apvts, "satBypass", satBypassButton);
    setupKnob(preGainSlider);
    setupKnob(postGainSlider);
    setupLabel(preGainLabel, "Pre-Gain");
    setupLabel(postGainLabel, "Post-Gain");

    preGainAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "preGain", preGainSlider);
    postGainAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "postGain", postGainSlider);

    // Chorus
    setupLabel(chorusLabel, "Chorus");
    detailContent.addAndMakeVisible(chorusBypassButton);
    chorusBypassAttach = std::make_unique<ButtonAttachment>(*audioProcessor.apvts, "chorusBypass", chorusBypassButton);
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

    lfoRateAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "lforate", lfoRateSlider);
    lfoDepthAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "lfodepth", lfoDepthSlider);
    centerDelayAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "centerdelay", centerDelaySlider);
    chorusFeedbackAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "chorfeedback", chorusFeedbackSlider);
    chorusMixAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "chormix", chorusMixSlider);

    // Reverb
    setupLabel(reverbLabel, "Reverb");
    detailContent.addAndMakeVisible(reverbBypassButton);
    reverbBypassAttach = std::make_unique<ButtonAttachment>(*audioProcessor.apvts, "reverbBypass", reverbBypassButton);
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

    roomSizeAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "roomSize", roomSizeSlider);
    dampingAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "damping", dampingSlider);
    reverbWidthAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "width", reverbWidthSlider);
    reverbWetAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "wet", reverbWetSlider);
    reverbDryAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "dry", reverbDrySlider);
    freezeModeAttach = std::make_unique<SliderAttachment>(*audioProcessor.apvts, "freeze", freezeModeSlider);

    // inspector = std::make_unique<melatonin::Inspector>(content);
    // addAndMakeVisible(*inspector);

    macroPage.addAndMakeVisible (webView);
    webView.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    if (auto* macroParam = audioProcessor.apvts->getParameter ("macro"))
    {
        macroWebAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
            *macroParam, macroSliderRelay, audioProcessor.apvts->undoManager);
    }

    resized();
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

    // --- Macro page: full-area WebView (React macro knob)
    webView.setBounds (macroPage.getLocalBounds());

    // --- Mapping Layout ---
    auto mapArea = mapPage.getLocalBounds().reduced(10);
    const int rowH = 26;
    const int pad = 4;

    for (auto* block : mappingBlocks)
    {
        // Row 1: section label + param dropdown
        auto row1 = mapArea.removeFromTop(rowH);
        block->sectionLabel.setBounds(row1.removeFromLeft(100));
        block->paramSelector.setBounds(row1.removeFromLeft(150));
        row1.removeFromLeft(pad);
        block->mapButton.setBounds(row1.removeFromLeft(45));
        row1.removeFromLeft(pad);
        block->unmapButton.setBounds(row1.removeFromLeft(60));

        mapArea.removeFromTop(2);

        // Row 2: min, max, curve
        auto row2 = mapArea.removeFromTop(rowH);
        block->minLabel.setBounds(row2.removeFromLeft(30));
        block->minSlider.setBounds(row2.removeFromLeft(130));
        row2.removeFromLeft(pad);
        block->maxLabel.setBounds(row2.removeFromLeft(32));
        block->maxSlider.setBounds(row2.removeFromLeft(130));
        row2.removeFromLeft(pad);
        block->curveLabel.setBounds(row2.removeFromLeft(40));
        block->curveSelector.setBounds(row2.removeFromLeft(100));

        mapArea.removeFromTop(pad + 4);
    }

    // --- Detailed Layout ---
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
    {
        auto hdr = eqArea.removeFromTop(labelHeight);
        eqBypassButton.setBounds(hdr.removeFromRight(90));
        eqLabel.setBounds(hdr);
    }
    auto colWidth = eqArea.getWidth() / 4;
    layoutColumn(lowFreqSlider, lowFreqLabel, lowGainSlider, lowGainLabel, lowQSlider, lowQLabel, eqArea.removeFromLeft(colWidth));
    layoutColumn(lowMidFreqSlider, lowMidFreqLabel, lowMidGainSlider, lowMidGainLabel, lowMidQSlider, lowMidQLabel, eqArea.removeFromLeft(colWidth));
    layoutColumn(highMidFreqSlider, highMidFreqLabel, highMidGainSlider, highMidGainLabel, highMidQSlider, highMidQLabel, eqArea.removeFromLeft(colWidth));
    layoutColumn(highFreqSlider, highFreqLabel, highGainSlider, highGainLabel, highQSlider, highQLabel, eqArea);

    // 2. Compressor Section
    auto compArea = fullArea.removeFromTop(150);
    {
        auto hdr = compArea.removeFromTop(labelHeight);
        compBypassButton.setBounds(hdr.removeFromRight(90));
        compLabel.setBounds(hdr);
    }
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
    {
        auto hdr = satArea.removeFromTop(labelHeight);
        satBypassButton.setBounds(hdr.removeFromRight(90));
        saturateLabel.setBounds(hdr);
    }
    int sx = satArea.getX();
    int sy = satArea.getY();
    preGainSlider.setBounds(sx, sy, knobSize, knobSize);
    preGainLabel.setBounds(sx, sy + knobSize, knobSize, labelHeight);
    sx += knobSize + spacing;
    postGainSlider.setBounds(sx, sy, knobSize, knobSize);
    postGainLabel.setBounds(sx, sy + knobSize, knobSize, labelHeight);

    // 4. Chorus Section
    auto chorArea = fullArea.removeFromTop(150);
    {
        auto hdr = chorArea.removeFromTop(labelHeight);
        chorusBypassButton.setBounds(hdr.removeFromRight(90));
        chorusLabel.setBounds(hdr);
    }
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
    {
        auto hdr = revArea.removeFromTop(labelHeight);
        reverbBypassButton.setBounds(hdr.removeFromRight(90));
        reverbLabel.setBounds(hdr);
    }
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

void SuperAwesomeVocalChainAudioProcessorEditor::setupMappingBlock (
    int index, const juce::String& title, std::vector<ParamEntry> params)
{
    auto* block = mappingBlocks.add (new MappingBlock());
    block->params = std::move (params);

    block->sectionLabel.setText (title, juce::dontSendNotification);
    block->sectionLabel.setFont (juce::Font (16.0f).boldened());
    block->sectionLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    mapPage.addAndMakeVisible (block->sectionLabel);

    for (int i = 0; i < (int) block->params.size(); ++i)
        block->paramSelector.addItem (block->params[i].displayName, i + 1);
    block->paramSelector.setSelectedId (1, juce::dontSendNotification);
    block->paramSelector.onChange = [this, index] { onParamSelected (index); };
    mapPage.addAndMakeVisible (block->paramSelector);

    block->minLabel.setText ("Min:", juce::dontSendNotification);
    block->minLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    mapPage.addAndMakeVisible (block->minLabel);

    block->maxLabel.setText ("Max:", juce::dontSendNotification);
    block->maxLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    mapPage.addAndMakeVisible (block->maxLabel);

    block->curveLabel.setText ("Curve:", juce::dontSendNotification);
    block->curveLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    mapPage.addAndMakeVisible (block->curveLabel);

    // Set min/max slider ranges from the selected param
    block->minSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    block->minSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 24);
    mapPage.addAndMakeVisible (block->minSlider);

    block->maxSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    block->maxSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 24);
    mapPage.addAndMakeVisible (block->maxSlider);

    block->curveSelector.addItem ("Linear", 1);
    block->curveSelector.addItem ("Logarithmic", 2);
    block->curveSelector.addItem ("Exponential", 3);
    block->curveSelector.setSelectedId (1, juce::dontSendNotification);
    mapPage.addAndMakeVisible (block->curveSelector);

    block->mapButton.onClick = [this, index] { onMapClicked (index); };
    mapPage.addAndMakeVisible (block->mapButton);

    block->unmapButton.onClick = [this, index] { onUnmapClicked (index); };
    mapPage.addAndMakeVisible (block->unmapButton);

    // Initialize slider ranges from the first param
    onParamSelected (index);
}

void SuperAwesomeVocalChainAudioProcessorEditor::onParamSelected (int blockIndex)
{
    auto* block = mappingBlocks[blockIndex];
    int sel = block->paramSelector.getSelectedId() - 1;
    if (sel < 0 || sel >= (int) block->params.size()) return;

    auto& paramID = block->params[sel].paramID;
    auto range = audioProcessor.apvts->getParameterRange (paramID);

    block->minSlider.setRange (range.start, range.end, 0.01);
    block->maxSlider.setRange (range.start, range.end, 0.01);

    // If this param is already mapped, load its mapping values
    if (audioProcessor.macroController)
    {
        for (auto& m : audioProcessor.macroController->getMappings())
        {
            if (m.targetParamID == paramID)
            {
                block->minSlider.setValue (m.minValue, juce::dontSendNotification);
                block->maxSlider.setValue (m.maxValue, juce::dontSendNotification);
                float curve = m.curve;
                if (curve <= 1.0f)      block->curveSelector.setSelectedId (1, juce::dontSendNotification);
                else if (curve <= 1.5f) block->curveSelector.setSelectedId (2, juce::dontSendNotification);
                else                    block->curveSelector.setSelectedId (3, juce::dontSendNotification);
                return;
            }
        }
    }

    // Not mapped — set defaults to param range endpoints
    block->minSlider.setValue (range.start, juce::dontSendNotification);
    block->maxSlider.setValue (range.end, juce::dontSendNotification);
}

void SuperAwesomeVocalChainAudioProcessorEditor::onMapClicked (int blockIndex)
{
    auto* block = mappingBlocks[blockIndex];
    int sel = block->paramSelector.getSelectedId() - 1;
    if (sel < 0 || sel >= (int) block->params.size()) return;

    float curve = 1.0f;
    int curveId = block->curveSelector.getSelectedId();
    if (curveId == 2) curve = 0.5f;  // logarithmic (sqrt shape)
    if (curveId == 3) curve = 2.0f;  // exponential

    MacroMapping newMapping;
    newMapping.targetParamID = block->params[sel].paramID;
    newMapping.minValue = (float) block->minSlider.getValue();
    newMapping.maxValue = (float) block->maxSlider.getValue();
    newMapping.curve = curve;

    if (audioProcessor.macroController)
    {
        // Get existing mappings, replace if same param, otherwise append
        auto mappings = audioProcessor.macroController->getMappings();
        bool found = false;
        for (auto& m : mappings)
        {
            if (m.targetParamID == newMapping.targetParamID)
            {
                m = newMapping;
                found = true;
                break;
            }
        }
        if (!found)
            mappings.push_back (newMapping);

        audioProcessor.macroController->setMappings (std::move (mappings));
    }

    refreshMappingHighlights();
}

void SuperAwesomeVocalChainAudioProcessorEditor::onUnmapClicked (int blockIndex)
{
    auto* block = mappingBlocks[blockIndex];
    int sel = block->paramSelector.getSelectedId() - 1;
    if (sel < 0 || sel >= (int) block->params.size()) return;

    auto paramID = block->params[sel].paramID;

    if (audioProcessor.macroController)
    {
        auto mappings = audioProcessor.macroController->getMappings();
        mappings.erase (
            std::remove_if (mappings.begin(), mappings.end(),
                [&](const MacroMapping& m) { return m.targetParamID == paramID; }),
            mappings.end());
        audioProcessor.macroController->setMappings (std::move (mappings));
    }

    refreshMappingHighlights();
}

void SuperAwesomeVocalChainAudioProcessorEditor::refreshMappingHighlights()
{
    // Collect currently mapped param IDs
    std::set<juce::String> mappedIDs;
    if (audioProcessor.macroController)
        for (auto& m : audioProcessor.macroController->getMappings())
            mappedIDs.insert (m.targetParamID);

    // Update each block's dropdown to show which params are mapped
    for (auto* block : mappingBlocks)
    {
        block->paramSelector.clear (juce::dontSendNotification);
        for (int i = 0; i < (int) block->params.size(); ++i)
        {
            auto name = block->params[i].displayName;
            if (mappedIDs.count (block->params[i].paramID))
                name += " *";
            block->paramSelector.addItem (name, i + 1);
        }
        // Restore selection
        if (block->paramSelector.getNumItems() > 0)
            block->paramSelector.setSelectedId (1, juce::dontSendNotification);
    }
}