#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
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
    "roomSize", "damping", "width", "wet", "dry",
    "fxChainOrder",
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
struct ParamSnapshot
{
    juce::String paramID;
    float value;
};

struct FactoryPreset
{
    juce::String name;
    std::vector<MacroMapping> mappings;
    std::vector<ParamSnapshot> paramValues = {};
    bool resetParameters = true;
};

const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets = {
        {
            "Aggressive Vocal",
            // Macro-driven mappings
            {
                { "lowMidGain",  0.0f,    7.0f, 1.0f, false },
                { "highMidGain", 0.0f,    9.0f, 1.0f, false },
                { "threshold",   0.0f,  -24.0f, 1.0f, false },
                { "ratio",       1.0f,    6.0f, 1.0f, false },
                { "preGain",     1.0f,    5.0f, 1.0f, false },
                { "postGain",    0.2f,    1.0f, 1.0f, true  },
            },
            // Static parameter snapshot
            {
                { "lowMidFreq",  1000.0f },
                { "highMidFreq", 3000.0f },
            },
        },
        {
            "Airy Vocal",
            {
                { "highGain",    0.0f,  10.0f, 1.0f, false },
                { "highMidGain", 0.0f,   8.0f, 1.0f, false },
                { "lowGain",    -8.0f,   0.0f, 1.0f, true  },
                { "wet",         0.0f,   1.0f, 1.0f, false },
                { "roomSize",    0.2f,  0.55f, 1.0f, false },
            },
            {
                { "highFreq",    6000.0f },
                { "highMidFreq", 3000.0f },
                { "lowFreq",      300.0f },
            },
        },
        {
            "Fuzzy Vocal",
            {
                { "highGain",    -15.0f, 0.0f,  1.0f, true  },
                { "highMidGain", -15.0f, 0.0f,  1.0f, true  },
                { "lowMidGain",    0.0f, 6.0f,  1.0f, false },
                { "wet",           0.1f, 0.5f,  1.0f, true  },
                { "dry",           0.5f, 0.9f,  1.0f, false },
                { "roomSize",      0.1f, 0.5f,  1.0f, true  },
                { "damping",       0.1f, 0.5f,  1.0f, false },
                { "preGain",       1.0f, 5.0f,  1.0f, false },
                { "chormix",       0.0f, 0.33f, 1.0f, true  },
            },
        },
    };
    return presets;
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

    if (auto* fxOrd = audioProcessor.apvts->getRawParameterValue ("fxChainOrder"))
        rootObj->setProperty (
            "fxChainOrder",
            (int) std::lround ((double) fxOrd->load()));

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
            mo->setProperty ("inverted", m.inverted);
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
                nm.inverted = obj->hasProperty ("inverted") && (bool) obj->getProperty ("inverted");

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

    o = o.withNativeFunction (
        juce::Identifier ("safc_getCurrentPresetName"),
        [this] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion ok)
        { ok ({ audioProcessor.lastPresetName }); });

    o = o.withNativeFunction (
        juce::Identifier ("safc_listPresets"),
        [] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion ok)
        {
            juce::Array<juce::var> arr;
            for (const auto& p : getFactoryPresets())
            {
                auto* obj = new juce::DynamicObject();
                obj->setProperty ("name", p.name);
                obj->setProperty ("builtIn", true);
                arr.add (juce::var (obj));
            }
            ok ({ juce::JSON::toString (juce::var (arr), true) });
        });

    o = o.withNativeFunction (
        juce::Identifier ("safc_loadPreset"),
        [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion ok)
        {
            if (args.isEmpty() || audioProcessor.macroController == nullptr)
            {
                ok ({ false });
                return;
            }

            const auto name = args[0].toString();
            for (const auto& p : getFactoryPresets())
            {
                if (p.name == name)
                {
                    // Clear mappings first so subsequent resets/sets don't trigger
                    // stale-mapping re-application via the macro listener.
                    audioProcessor.macroController->setMappings ({});

                    if (p.resetParameters)
                        for (auto* param : audioProcessor.getParameters())
                            param->setValueNotifyingHost (param->getDefaultValue());

                    for (const auto& pv : p.paramValues)
                    {
                        if (auto* rap = audioProcessor.apvts->getParameter (pv.paramID))
                            rap->setValueNotifyingHost (rap->getNormalisableRange().convertTo0to1 (pv.value));
                    }

                    audioProcessor.macroController->setMappings (p.mappings);
                    audioProcessor.lastPresetName = name;
                    ok ({ true });
                    return;
                }
            }
            ok ({ false });
        });

    o = o.withNativeFunction (
        juce::Identifier ("safc_getMeters"),
        [this] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion ok)
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("input",  (double) audioProcessor.meterInputPeak.load());
            obj->setProperty ("output", (double) audioProcessor.meterOutputPeak.load());
            ok ({ juce::JSON::toString (juce::var (obj), false) });
        });

    o = o.withNativeFunction (
        juce::Identifier ("safc_resetAll"),
        [this] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion ok)
        {
            // Clear mappings first so resetting `macro` doesn't trigger stale-mapping re-application.
            if (audioProcessor.macroController != nullptr)
                audioProcessor.macroController->setMappings ({});

            for (auto* param : audioProcessor.getParameters())
                param->setValueNotifyingHost (param->getDefaultValue());

            audioProcessor.lastPresetName = {};
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

    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
    content.setWantsKeyboardFocus (false);
    content.setMouseClickGrabsKeyboardFocus (false);
    webView->setWantsKeyboardFocus (false);
    webView->setMouseClickGrabsKeyboardFocus (false);

    addAndMakeVisible (content);
    content.addAndMakeVisible (*webView);

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

    if (webView != nullptr)
        webView->setBounds (content.getLocalBounds());
}

void SuperAwesomeVocalChainAudioProcessorEditor::timerCallback()
{
    // Message-thread meter decay: keeps the atomics falling even when the host
    // stops calling processBlock (transport stopped, audio idle, etc.).
    // Audio thread still wins via `max(peak, prev * 0.96)` when signal is present.
    constexpr float kIdleDecay = 0.86f; // applied per 50ms tick → ~12 dB/s
    audioProcessor.meterInputPeak.store (
        audioProcessor.meterInputPeak.load (std::memory_order_relaxed) * kIdleDecay,
        std::memory_order_relaxed);
    audioProcessor.meterOutputPeak.store (
        audioProcessor.meterOutputPeak.load (std::memory_order_relaxed) * kIdleDecay,
        std::memory_order_relaxed);
}

void SuperAwesomeVocalChainAudioProcessorEditor::updateVisibility() {}
