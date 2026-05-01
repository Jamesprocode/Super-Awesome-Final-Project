#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <vector>

namespace
{

constexpr const char* kSliderRelayIds[] = {
    "macro",
    "inputGain", "outputGain", "outputDryWet",
    "lowFreq", "lowGain", "lowQ",
    "lowMidFreq", "lowMidGain", "lowMidQ",
    "highMidFreq", "highMidGain", "highMidQ",
    "highFreq", "highGain", "highQ",
    "threshold", "ratio", "attack", "release",
    "preGain", "postGain",
    "lforate", "lfodepth", "centerdelay", "chorfeedback", "chormix",
    "roomSize", "damping", "width", "wet", "dry"
};

constexpr const char* kToggleRelayIds[] = {
    "allFxBypass",
    "eqBypass", "compBypass", "satBypass", "chorusBypass", "reverbBypass", "freeze"
};

//==============================================================================
juce::File getUiPublicFolder()
{
    return juce::File { __FILE__ }
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
    if (extLowerNoDot == "jpg")   return "image/jpeg";
    if (extLowerNoDot == "jpeg")  return "image/jpeg";
    if (extLowerNoDot == "svg")   return "image/svg+xml";
    if (extLowerNoDot == "ico")   return "image/vnd.microsoft.icon";
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

void appendMappingBlocks (juce::Array<juce::var>& blocksOut, juce::AudioProcessorValueTreeState& apvts)
{
    auto addBlock = [&] (const juce::String& title,
                         std::initializer_list<std::pair<const char*, const char*>> items)
    {
        auto blockObj = new juce::DynamicObject();
        blockObj->setProperty ("title", title);
        juce::Array<juce::var> params;
        for (auto [id, label] : items)
        {
            auto p = new juce::DynamicObject();
            p->setProperty ("id", id);
            p->setProperty ("label", label);
            const auto r = apvts.getParameterRange (id);
            p->setProperty ("rangeMin", (double) r.start);
            p->setProperty ("rangeMax", (double) r.end);
            params.add (juce::var (p));
        }
        blockObj->setProperty ("params", juce::var (params));
        blocksOut.add (juce::var (blockObj));
    };

    addBlock ("EQ", {
        {"lowFreq", "Low Freq (Hz)"}, {"lowGain", "Low Gain (dB)"}, {"lowQ", "Low Q"},
        {"lowMidFreq", "Low-Mid Freq (Hz)"}, {"lowMidGain", "Low-Mid Gain (dB)"}, {"lowMidQ", "Low-Mid Q"},
        {"highMidFreq", "High-Mid Freq (Hz)"}, {"highMidGain", "High-Mid Gain (dB)"}, {"highMidQ", "High-Mid Q"},
        {"highFreq", "High Freq (Hz)"}, {"highGain", "High Gain (dB)"}, {"highQ", "High Q"}});

    addBlock ("Compressor", {
        {"threshold", "Threshold (dB)"}, {"ratio", "Ratio"}, {"attack", "Attack (ms)"}, {"release", "Release (ms)"}});

    addBlock ("Saturator", {
        {"preGain", "Pre-Gain (dB)"}, {"postGain", "Post-Gain (dB)"}});

    addBlock ("Chorus", {
        {"lforate", "LFO Rate (Hz)"}, {"lfodepth", "LFO Depth (%)"}, {"centerdelay", "Center Delay (ms)"},
        {"chorfeedback", "Feedback (%)"}, {"chormix", "Mix (%)"}});

    addBlock ("Reverb", {
        {"roomSize", "Room Size"}, {"damping", "Damping"}, {"width", "Width"},
        {"wet", "Wet"}, {"dry", "Dry"}, {"freeze", "Freeze"}});
}

float curveExponentFromShapeId (int curveShapeId)
{
    switch (curveShapeId)
    {
        case 2: return 0.5f;
        case 3: return 2.0f;
        default: break;
    }
    return 1.0f;
}

//==============================================================================
} // namespace

//==============================================================================
juce::String SuperAwesomeVocalChainAudioProcessorEditor::getMappingStateJson() const
{
    auto* rootObj = new juce::DynamicObject();
    juce::Array<juce::var> blocks;
    appendMappingBlocks (blocks, *audioProcessor.apvts);
    rootObj->setProperty ("blocks", juce::var (blocks));

    juce::Array<juce::var> mappings;
    if (audioProcessor.macroController != nullptr)
    {
        for (const auto& m : audioProcessor.macroController->getMappings())
        {
            auto* mo = new juce::DynamicObject();
            mo->setProperty ("targetParamID", m.targetParamID);
            mo->setProperty ("minValue", m.minValue);
            mo->setProperty ("maxValue", m.maxValue);
            mo->setProperty ("curveExponent", m.curve);
            mappings.add (juce::var (mo));
        }
    }
    rootObj->setProperty ("mappings", juce::var (mappings));

    return juce::JSON::toString (juce::var (rootObj), true);
}

//==============================================================================
juce::WebBrowserComponent::Options SuperAwesomeVocalChainAudioProcessorEditor::buildWebViewOptions()
{
    auto o = juce::WebBrowserComponent::Options()
                 .withNativeIntegrationEnabled (true)
                 .withResourceProvider ([this] (const juce::String& url) { return getResource (url); })
                 .withKeepPageLoadedWhenBrowserIsHidden(); // SPA stays responsive when swapping JUCE tabs

    for (auto& relay : webSliderRelays)
        o = o.withOptionsFrom (*relay);

    for (auto& relay : webToggleRelays)
        o = o.withOptionsFrom (*relay);

    o = o.withNativeFunction (
        juce::Identifier ("safc_getMappingStateJson"),
        [this] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion ok)
        { ok ({ getMappingStateJson() }); });

    o = o.withNativeFunction (
        juce::Identifier ("safc_applyMacroMapping"),
        [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion ok)
        {
            if (args.isEmpty())
            {
                ok ({});
                return;
            }

            if (auto* obj = args[0].getDynamicObject())
            {
                const auto paramID = obj->getProperty ("targetParamID").toString().trim();

                MacroMapping nm;
                nm.targetParamID = paramID;
                nm.minValue = (float) obj->getProperty ("minValue");
                nm.maxValue = (float) obj->getProperty ("maxValue");
                nm.curve = curveExponentFromShapeId ((int) obj->getProperty ("curveShape"));

                if (paramID.isNotEmpty() && audioProcessor.macroController != nullptr)
                {
                    auto mappings = audioProcessor.macroController->getMappings();
                    bool found = false;
                    for (auto& m : mappings)
                    {
                        if (m.targetParamID == paramID)
                        {
                            m = nm;
                            found = true;
                            break;
                        }
                    }
                    if (! found)
                        mappings.push_back (nm);

                    audioProcessor.macroController->setMappings (std::move (mappings));
                }
            }
            ok ({ true });
        });

    o = o.withNativeFunction (
        juce::Identifier ("safc_removeMacroMapping"),
        [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion ok)
        {
            if (args.isEmpty())
            {
                ok ({});
                return;
            }

            const auto pid = args[0].toString().trim();
            if (pid.isNotEmpty() && audioProcessor.macroController != nullptr)
            {
                auto mappings = audioProcessor.macroController->getMappings();
                mappings.erase (
                    std::remove_if (
                        mappings.begin(), mappings.end(),
                        [&pid] (const MacroMapping& m) { return m.targetParamID == pid; }),
                    mappings.end());

                audioProcessor.macroController->setMappings (std::move (mappings));
            }
            ok ({ true });
        });

   #if JUCE_WINDOWS
    o = o.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2()
            .withBackgroundColour (juce::Colours::white));
   #endif

    return o;
}

std::optional<SuperAwesomeVocalChainAudioProcessorEditor::Resource> SuperAwesomeVocalChainAudioProcessorEditor::getResource (
    const juce::String& url)
{
   #if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    const juce::String rel = url == "/" || url.isEmpty()
        ? "index.html"
        : juce::String { url.fromFirstOccurrenceOf ("/", false, false) };

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
SuperAwesomeVocalChainAudioProcessorEditor::SuperAwesomeVocalChainAudioProcessorEditor (
    SuperAwesomeVocalChainAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (1000, 600);

    for (auto id : kSliderRelayIds)
        webSliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (id));

    for (auto id : kToggleRelayIds)
        webToggleRelays.push_back (std::make_unique<juce::WebToggleButtonRelay> (id));

    webView = std::make_unique<juce::WebBrowserComponent> (buildWebViewOptions());

    addAndMakeVisible (content);
    content.addAndMakeVisible (*webView);
    content.addAndMakeVisible (macroTab);
    content.addAndMakeVisible (mapTab);
    content.addAndMakeVisible (detailTab);

    macroTab.onClick = [this] { showPage (macroPageIndex); };
    mapTab.onClick = [this] { showPage (mapPageIndex); };
    detailTab.onClick = [this] { showPage (detailedPageIndex); };

    juce::UndoManager* undo = nullptr;

    jassert ((size_t) webSliderRelays.size() == std::size (kSliderRelayIds));

    webSliderAttachments.clear();
    for (size_t i = 0; i < webSliderRelays.size(); ++i)
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (audioProcessor.apvts->getParameter (
            kSliderRelayIds[i]));
        if (ranged != nullptr)
            webSliderAttachments.push_back (std::make_unique<juce::WebSliderParameterAttachment> (
                *ranged, *webSliderRelays[static_cast<size_t> (i)], undo));
    }

    for (size_t i = 0; i < webToggleRelays.size(); ++i)
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (audioProcessor.apvts->getParameter (
            kToggleRelayIds[i]));
        if (ranged != nullptr)
            webToggleAttachments.push_back (std::make_unique<juce::WebToggleButtonParameterAttachment> (
                *ranged, *webToggleRelays[static_cast<size_t> (i)], undo));
    }

    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    showPage (macroPageIndex);

    resized();
    startTimer (50);
}

SuperAwesomeVocalChainAudioProcessorEditor::~SuperAwesomeVocalChainAudioProcessorEditor() = default;

void SuperAwesomeVocalChainAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void SuperAwesomeVocalChainAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    const int inspectorWidth = 350;
    if (inspector != nullptr)
        inspector->setBounds (area.removeFromRight (inspectorWidth));

    content.setBounds (area);
    auto bounds = content.getLocalBounds();

    const int tabHeight = 50;
    auto tabs = bounds.removeFromBottom (tabHeight);
    auto tabThird = tabs.getWidth() / 3;
    macroTab.setBounds (tabs.removeFromLeft (tabThird));
    mapTab.setBounds (tabs.removeFromLeft (tabThird));
    detailTab.setBounds (tabs);

    if (webView != nullptr)
        webView->setBounds (bounds);
}

void SuperAwesomeVocalChainAudioProcessorEditor::showPage (int index)
{
    macroTab.setToggleState (index == macroPageIndex, juce::dontSendNotification);
    mapTab.setToggleState (index == mapPageIndex, juce::dontSendNotification);
    detailTab.setToggleState (index == detailedPageIndex, juce::dontSendNotification);

    if (webView == nullptr)
        return;

    webView->evaluateJavascript (
        "(function(){try{var t=window.__SAFC_SET_TAB__;if(typeof t==='function')t(" + juce::String (index)
        + ");}catch(e){}})();");
}

void SuperAwesomeVocalChainAudioProcessorEditor::timerCallback()
{
    updateVisibility();
    stopTimer();
}

void SuperAwesomeVocalChainAudioProcessorEditor::updateVisibility() {}
