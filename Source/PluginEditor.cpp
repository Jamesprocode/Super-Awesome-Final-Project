#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <SAFCWebAssets.h>
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
    "satType",
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

juce::String getResourcePathFromUrl (const juce::String& url)
{
    auto rel = url == "/" || url.isEmpty()
        ? juce::String { "index.html" }
        : juce::String { url.fromFirstOccurrenceOf ("/", false, false) };

    rel = rel.upToFirstOccurrenceOf ("?", false, false)
             .upToFirstOccurrenceOf ("#", false, false)
             .replaceCharacter ('\\', '/');

    while (rel.startsWith ("/"))
        rel = rel.substring (1);

    return rel;
}

bool isSafeResourcePath (const juce::String& rel)
{
    return rel.isNotEmpty()
        && ! rel.contains ("/../")
        && ! rel.startsWith ("..")
        && ! juce::File::isAbsolutePath (rel);
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

juce::String getExtensionWithoutDot (const juce::String& filename)
{
    auto ext = juce::File { filename }.getFileExtension().toLowerCase();
    if (ext.isNotEmpty() && ext[0] == '.')
        ext = ext.fromFirstOccurrenceOf (".", false, false);

    return ext;
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

std::optional<juce::WebBrowserComponent::Resource> getEmbeddedUiResource (const juce::String& rel)
{
    const auto requestedFilename = rel.fromLastOccurrenceOf ("/", false, false);

    for (int i = 0; i < SAFCWebAssets::namedResourceListSize; ++i)
    {
        if (requestedFilename != SAFCWebAssets::originalFilenames[i])
            continue;

        int dataSize = 0;
        const auto* data = SAFCWebAssets::getNamedResource (SAFCWebAssets::namedResourceList[i], dataSize);
        if (data == nullptr || dataSize < 0)
            return std::nullopt;

        std::vector<std::byte> bytes ((size_t) dataSize);
        if (dataSize > 0)
            std::memcpy (bytes.data(), data, (size_t) dataSize);

        return juce::WebBrowserComponent::Resource {
            std::move (bytes),
            getMimeTypeForExtension (getExtensionWithoutDot (requestedFilename))
        };
    }

    return std::nullopt;
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

/** Saturator transfer-function indices — must mirror the StringArray order in createParameterLayout. */
namespace SatType {
    enum : int
    {
        Cubic = 0,
        Soft  = 1,
        Tape  = 2,
        Tube  = 3,
        Hard  = 4
    };
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
                { "preGain",     1.0f,    3.0f, 1.0f, false },
                { "postGain",    0.5f,    1.0f, 1.0f, true  },
            },
            // Static parameter snapshot
            {
                { "lowMidFreq",  1000.0f },
                { "highMidFreq", 3000.0f },
                { "satType", (float) SatType::Hard },
            },
        },
        {
            "Airy Vocal",
            {
                { "highGain",    0.0f,  10.0f, 1.0f, false },
                { "highMidGain", 0.0f,   5.0f, 1.0f, false },
                { "lowGain",    -8.0f,   0.0f, 1.0f, true  },
                { "wet",         0.0f,   0.8f, 1.0f, false },
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
                { "preGain",       1.0f, 3.0f,  1.0f, false },
                { "postGain",    0.5f,    1.0f, 1.0f, true  },
                { "chormix",       0.0f, 0.33f, 1.0f, true  },
            },
            {
                { "satType", (float) SatType::Tape },
            },
        },
        {
            "Warm Vocal",
            {
                // EQ
                { "lowMidGain",  0.0f,    6.0f, 1.0f, false },
                { "highGain",    0.0f,   -2.0f, 1.0f, false },
                // Compressor
                { "threshold",   0.0f,  -12.0f, 1.0f, false },
                // Saturator — Tube drive with output compensation
                { "preGain",     1.0f,    3.0f, 1.0f, false },
                { "postGain",   0.33f,    1.0f, 1.0f, true  },
                // Reverb
                { "roomSize",    0.0f,    0.5f, 1.0f, false },
                { "wet",         0.0f,    0.5f, 1.0f, false },
            },
            // Static snapshot
            {
                { "lowMidFreq",   500.0f },
                { "highFreq",    7000.0f },
                { "ratio",          4.0f },
                { "attack",       200.0f },
                { "release",       30.0f },
                { "satType", (float) SatType::Tube },
            },
        },
        {
            "Spooky Vocal",
            {
                // EQ — high shelf cut (logarithmic, inverse) + low shelf thin (inverse)
                { "highGain",    -24.0f,  0.0f,  0.5f, true  },
                { "lowGain",     -12.0f,  0.0f,  1.0f, true  },
                // Saturator — drive into mild saturation, compensate output
                { "preGain",       1.0f,  1.8f,  1.0f, false },
                { "postGain",      0.5f,  1.0f,  1.0f, true  },
                // Chorus — heavier mix and feedback for the spooky wash
                { "chorfeedback", -1.0f,  0.6f,  1.0f, false },
                { "chormix",       0.0f,  0.6f,  1.0f, false },
                // Reverb
                { "roomSize",      0.0f,  0.9f,  1.0f, false },
                { "width",         0.0f,  0.77f, 1.0f, false },
                { "wet",           0.0f,  0.65f, 0.5f, false },
            },
        },
    };
    return presets;
}

//==============================================================================
constexpr const char* kUserPresetRootType = "SAFCUserPreset";
constexpr const char* kParameterValuesType = "ParameterValues";
constexpr const char* kParameterValueType = "Param";
constexpr const char* kMacroMappingsType = "MacroMappings";
constexpr const char* kMacroMappingType = "Mapping";
constexpr const char* kUserPresetExtension = "safcpreset";

juce::String makePresetId (const juce::String& source, const juce::String& name)
{
    return source + ":" + name;
}

juce::File getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("SuperAwesome")
        .getChildFile ("Super Awesome Vocal Chain")
        .getChildFile ("Presets");
}

juce::String normaliseUserPresetName (juce::String name)
{
    name = name.replaceCharacters ("\r\n\t", "   ").trim();
    while (name.contains ("  "))
        name = name.replace ("  ", " ");

    return name.substring (0, 80).trim();
}

juce::File getUserPresetFileForName (const juce::String& name)
{
    auto legalName = juce::File::createLegalFileName (name);
    if (legalName.isEmpty())
        legalName = "Untitled";

    return getUserPresetFolder().getChildFile (legalName).withFileExtension (kUserPresetExtension);
}

juce::String getUserPresetNameFromFile (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml != nullptr)
    {
        const auto root = juce::ValueTree::fromXml (*xml);
        if (root.isValid() && root.hasType (kUserPresetRootType))
        {
            const auto name = root.getProperty ("name").toString().trim();
            if (name.isNotEmpty())
                return name;
        }
    }

    return file.getFileNameWithoutExtension();
}

struct UserPresetInfo
{
    juce::String name;
    juce::File file;
};

std::vector<UserPresetInfo> getUserPresetInfos()
{
    std::vector<UserPresetInfo> infos;
    const auto folder = getUserPresetFolder();
    if (! folder.exists())
        return infos;

    juce::Array<juce::File> files;
    folder.findChildFiles (
        files, juce::File::findFiles, false, juce::String ("*.") + kUserPresetExtension);

    for (const auto& file : files)
    {
        const auto name = normaliseUserPresetName (getUserPresetNameFromFile (file));
        if (name.isNotEmpty())
            infos.push_back ({ name, file });
    }

    std::sort (
        infos.begin(), infos.end(),
        [] (const UserPresetInfo& a, const UserPresetInfo& b)
        { return a.name.compareIgnoreCase (b.name) < 0; });

    return infos;
}

juce::File findUserPresetFile (const juce::String& rawName)
{
    const auto name = normaliseUserPresetName (rawName);
    if (name.isEmpty())
        return {};

    for (const auto& info : getUserPresetInfos())
        if (info.name == name)
            return info.file;

    const auto fallback = getUserPresetFileForName (name);
    return fallback.existsAsFile() ? fallback : juce::File {};
}

juce::ValueTree createParameterValuesValueTree (juce::AudioProcessorValueTreeState& apvts)
{
    juce::StringArray ids;
    for (auto id : kSliderRelayIds)
        ids.addIfNotAlreadyThere (id);
    for (auto id : kToggleRelayIds)
        ids.addIfNotAlreadyThere (id);

    juce::ValueTree values (kParameterValuesType);
    for (const auto& id : ids)
    {
        if (auto* raw = apvts.getRawParameterValue (id))
        {
            juce::ValueTree pv (kParameterValueType);
            pv.setProperty ("id", id, nullptr);
            pv.setProperty ("value", (double) raw->load(), nullptr);
            values.appendChild (pv, nullptr);
        }
    }

    return values;
}

juce::ValueTree createMacroMappingsValueTree (const MacroController* macroController)
{
    juce::ValueTree mappingsVT (kMacroMappingsType);
    if (macroController == nullptr)
        return mappingsVT;

    for (const auto& m : macroController->getMappings())
    {
        juce::ValueTree mvt (kMacroMappingType);
        mvt.setProperty ("targetParamID", m.targetParamID, nullptr);
        mvt.setProperty ("minValue",      m.minValue,      nullptr);
        mvt.setProperty ("maxValue",      m.maxValue,      nullptr);
        mvt.setProperty ("curve",         m.curve,         nullptr);
        mvt.setProperty ("inverted",      m.inverted,      nullptr);
        mappingsVT.appendChild (mvt, nullptr);
    }

    return mappingsVT;
}

std::vector<MacroMapping> readMacroMappingsValueTree (const juce::ValueTree& mappingsVT)
{
    std::vector<MacroMapping> mappings;
    if (! mappingsVT.isValid())
        return mappings;

    for (auto child : mappingsVT)
    {
        if (! child.hasType (kMacroMappingType))
            continue;

        MacroMapping m;
        m.targetParamID = child.getProperty ("targetParamID").toString();
        m.minValue      = (float) child.getProperty ("minValue");
        m.maxValue      = (float) child.getProperty ("maxValue");
        m.curve         = (float) child.getProperty ("curve");
        m.inverted      = (bool)  child.getProperty ("inverted");

        if (m.targetParamID.isNotEmpty())
            mappings.push_back (m);
    }

    return mappings;
}

void setParameterPlainValue (
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& paramID,
    float plainValue)
{
    if (auto* ranged = apvts.getParameter (paramID))
    {
        const auto& range = ranged->getNormalisableRange();
        const auto normalised = juce::jlimit (0.0f, 1.0f, range.convertTo0to1 (plainValue));
        ranged->setValueNotifyingHost (normalised);
    }
}

void resetParametersForPreset (SuperAwesomeVocalChainAudioProcessor& processor)
{
    if (processor.macroController != nullptr)
        processor.macroController->setMappings ({});

    for (auto* param : processor.getParameters())
        param->setValueNotifyingHost (param->getDefaultValue());
}

bool applyFactoryPreset (
    SuperAwesomeVocalChainAudioProcessor& processor,
    const FactoryPreset& preset)
{
    if (processor.macroController == nullptr)
        return false;

    processor.macroController->setMappings ({});

    if (preset.resetParameters)
        for (auto* param : processor.getParameters())
            param->setValueNotifyingHost (param->getDefaultValue());

    for (const auto& pv : preset.paramValues)
        setParameterPlainValue (*processor.apvts, pv.paramID, pv.value);

    processor.macroController->setMappings (preset.mappings);
    processor.lastPresetName = preset.name;
    processor.lastPresetSource = "factory";
    return true;
}

juce::ValueTree createUserPresetValueTree (
    SuperAwesomeVocalChainAudioProcessor& processor,
    const juce::String& name)
{
    juce::ValueTree root (kUserPresetRootType);
    root.setProperty ("formatVersion", 1, nullptr);
    root.setProperty ("name", name, nullptr);
    root.appendChild (createParameterValuesValueTree (*processor.apvts), nullptr);
    root.appendChild (createMacroMappingsValueTree (processor.macroController.get()), nullptr);
    return root;
}

bool applyUserPresetValueTree (
    SuperAwesomeVocalChainAudioProcessor& processor,
    const juce::ValueTree& root)
{
    if (! root.isValid() || ! root.hasType (kUserPresetRootType))
        return false;

    resetParametersForPreset (processor);

    const auto valuesVT = root.getChildWithName (kParameterValuesType);
    if (valuesVT.isValid())
    {
        for (auto child : valuesVT)
        {
            if (! child.hasType (kParameterValueType))
                continue;

            const auto id = child.getProperty ("id").toString();
            if (id.isNotEmpty())
                setParameterPlainValue (*processor.apvts, id, (float) child.getProperty ("value"));
        }
    }

    if (processor.macroController != nullptr)
        processor.macroController->setMappings (
            readMacroMappingsValueTree (root.getChildWithName (kMacroMappingsType)));

    return true;
}

juce::String makePresetResponseJson (
    bool success,
    const juce::String& message = {},
    const juce::String& name = {},
    const juce::String& source = {})
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("ok", success);
    obj->setProperty ("message", message);
    obj->setProperty ("name", name);
    obj->setProperty ("source", source);
    if (source.isNotEmpty() && name.isNotEmpty())
        obj->setProperty ("id", makePresetId (source, name));

    return juce::JSON::toString (juce::var (obj), false);
}

bool saveUserPreset (
    SuperAwesomeVocalChainAudioProcessor& processor,
    const juce::String& rawName,
    juce::String& savedNameOut,
    juce::String& errorOut)
{
    const auto name = normaliseUserPresetName (rawName);
    if (name.isEmpty())
    {
        errorOut = "Enter a preset name.";
        return false;
    }

    auto folder = getUserPresetFolder();
    const auto dirResult = folder.createDirectory();
    if (dirResult.failed())
    {
        errorOut = dirResult.getErrorMessage();
        return false;
    }

    const auto file = getUserPresetFileForName (name);
    auto root = createUserPresetValueTree (processor, name);
    auto xml = root.createXml();
    if (xml == nullptr || ! file.replaceWithText (xml->toString(), false, false, "\n"))
    {
        errorOut = "Could not write preset file.";
        return false;
    }

    processor.lastPresetName = name;
    processor.lastPresetSource = "user";
    savedNameOut = name;
    return true;
}

bool loadUserPreset (
    SuperAwesomeVocalChainAudioProcessor& processor,
    const juce::String& rawName,
    juce::String& loadedNameOut,
    juce::String& errorOut)
{
    const auto file = findUserPresetFile (rawName);
    if (! file.existsAsFile())
    {
        errorOut = "Preset file was not found.";
        return false;
    }

    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr)
    {
        errorOut = "Preset file could not be read.";
        return false;
    }

    const auto root = juce::ValueTree::fromXml (*xml);
    if (! applyUserPresetValueTree (processor, root))
    {
        errorOut = "Preset file is not valid.";
        return false;
    }

    const auto storedName = normaliseUserPresetName (root.getProperty ("name").toString());
    loadedNameOut = storedName.isNotEmpty() ? storedName : normaliseUserPresetName (rawName);
    processor.lastPresetName = loadedNameOut;
    processor.lastPresetSource = "user";
    return true;
}

bool deleteUserPreset (const juce::String& rawName, juce::String& deletedNameOut, juce::String& errorOut)
{
    const auto name = normaliseUserPresetName (rawName);
    const auto file = findUserPresetFile (name);
    if (! file.existsAsFile())
    {
        errorOut = "Preset file was not found.";
        return false;
    }

    if (! file.deleteFile())
    {
        errorOut = "Could not delete preset file.";
        return false;
    }

    deletedNameOut = name;
    return true;
}

juce::String getCurrentPresetJson (const SuperAwesomeVocalChainAudioProcessor& processor)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("name", processor.lastPresetName);
    obj->setProperty ("source", processor.lastPresetSource);
    if (processor.lastPresetName.isNotEmpty() && processor.lastPresetSource.isNotEmpty())
        obj->setProperty ("id", makePresetId (processor.lastPresetSource, processor.lastPresetName));

    return juce::JSON::toString (juce::var (obj), false);
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
        juce::Identifier ("safc_getCurrentPresetJson"),
        [this] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion ok)
        { ok ({ getCurrentPresetJson (audioProcessor) }); });

    o = o.withNativeFunction (
        juce::Identifier ("safc_listPresets"),
        [] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion ok)
        {
            juce::Array<juce::var> arr;
            for (const auto& p : getFactoryPresets())
            {
                auto* obj = new juce::DynamicObject();
                obj->setProperty ("id", makePresetId ("factory", p.name));
                obj->setProperty ("name", p.name);
                obj->setProperty ("source", "factory");
                obj->setProperty ("builtIn", true);
                arr.add (juce::var (obj));
            }
            for (const auto& p : getUserPresetInfos())
            {
                auto* obj = new juce::DynamicObject();
                obj->setProperty ("id", makePresetId ("user", p.name));
                obj->setProperty ("name", p.name);
                obj->setProperty ("source", "user");
                obj->setProperty ("builtIn", false);
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
            const auto source = args.size() > 1 ? args[1].toString() : juce::String {};

            if (source == "user")
            {
                juce::String loadedName, error;
                const auto success = loadUserPreset (audioProcessor, name, loadedName, error);
                ok ({ success });
                return;
            }

            if (source != "user")
            {
                for (const auto& p : getFactoryPresets())
                {
                    if (p.name == name)
                    {
                        ok ({ applyFactoryPreset (audioProcessor, p) });
                        return;
                    }
                }
            }

            juce::String loadedName, error;
            if (loadUserPreset (audioProcessor, name, loadedName, error))
            {
                ok ({ true });
                return;
            }

            ok ({ false });
        });

    o = o.withNativeFunction (
        juce::Identifier ("safc_saveUserPreset"),
        [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion ok)
        {
            const auto name = args.isEmpty() ? juce::String {} : args[0].toString();
            juce::String savedName, error;
            if (saveUserPreset (audioProcessor, name, savedName, error))
                ok ({ makePresetResponseJson (true, {}, savedName, "user") });
            else
                ok ({ makePresetResponseJson (false, error) });
        });

    o = o.withNativeFunction (
        juce::Identifier ("safc_deleteUserPreset"),
        [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion ok)
        {
            const auto name = args.isEmpty() ? juce::String {} : args[0].toString();
            juce::String deletedName, error;
            if (deleteUserPreset (name, deletedName, error))
            {
                if (audioProcessor.lastPresetSource == "user" && audioProcessor.lastPresetName == deletedName)
                {
                    audioProcessor.lastPresetName = {};
                    audioProcessor.lastPresetSource = {};
                }
                ok ({ makePresetResponseJson (true, {}, deletedName, "user") });
            }
            else
            {
                ok ({ makePresetResponseJson (false, error) });
            }
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
            audioProcessor.lastPresetSource = {};
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
    const auto rel = getResourcePathFromUrl (url);

    if (! isSafeResourcePath (rel))
        return std::nullopt;

    if (auto embedded = getEmbeddedUiResource (rel))
        return embedded;

    const auto publicDir = getUiPublicFolder();
    const juce::File file = publicDir.getChildFile (rel);

    if (! file.isAChildOf (publicDir) || ! file.existsAsFile())
        return std::nullopt;

    auto bytes = loadFileToByteVector (file);
    if (file.getSize() > 0 && bytes.empty())
        return std::nullopt;

    return Resource { std::move (bytes), getMimeTypeForExtension (getExtensionWithoutDot (file.getFileName())) };
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
