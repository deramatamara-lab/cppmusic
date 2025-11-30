#include "PluginDatabase.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>
#include <filesystem>

namespace daw::plugins
{

void PluginDatabase::scanPluginDirectory(const std::string& pluginDirectory)
{
    if (pluginDirectory.empty())
        return;

    const std::filesystem::path directoryPath{ pluginDirectory };
    if (! std::filesystem::exists (directoryPath) || ! std::filesystem::is_directory (directoryPath))
        return;

    juce::AudioPluginFormatManager formatManager;
    formatManager.addDefaultFormats();

    juce::KnownPluginList pluginList;

    // Scan each file in the directory for available plugin types
    for (const auto& entry : std::filesystem::recursive_directory_iterator (directoryPath))
    {
        if (! entry.is_regular_file())
            continue;

        const auto filePath = entry.path().string();

        for (int i = 0; i < formatManager.getNumFormats(); ++i)
        {
            if (auto* format = formatManager.getFormat (i))
            {
                juce::OwnedArray<juce::PluginDescription> typesFound;
                pluginList.scanAndAddFile (filePath,
                                           true,
                                           typesFound,
                                           *format);
            }
        }
    }

    // Extract capabilities from scanned plugins
    auto types = pluginList.getTypes();
    for (const auto& pluginType : types)
    {
        PluginCapabilities caps;
        caps.pluginId = pluginType.createIdentifierString().toStdString();
        caps.name = pluginType.name.toStdString();
        caps.format = pluginType.pluginFormatName.toStdString();
        caps.path = pluginType.fileOrIdentifier.toStdString();
        caps.maxInputChannels = pluginType.numInputChannels;
        caps.maxOutputChannels = pluginType.numOutputChannels;
        caps.supportsMIDI = pluginType.isInstrument;
        caps.version = pluginType.version.toStdString();

        database[caps.pluginId] = caps;
    }
}

const PluginDatabase::PluginCapabilities* PluginDatabase::getCapabilities(const std::string& pluginId) const noexcept
{
    const auto it = database.find(pluginId);
    return it != database.end() ? &it->second : nullptr;
}

void PluginDatabase::setCapabilities(const PluginCapabilities& capabilities)
{
    database[capabilities.pluginId] = capabilities;
}

std::vector<std::string> PluginDatabase::getAllPluginIds() const noexcept
{
    std::vector<std::string> ids;
    ids.reserve(database.size());

    for (const auto& pair : database)
    {
        ids.push_back(pair.first);
    }

    return ids;
}

void PluginDatabase::clear() noexcept
{
    database.clear();
}

} // namespace daw::plugins
