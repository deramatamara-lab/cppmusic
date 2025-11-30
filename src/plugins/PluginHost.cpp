#include "PluginHost.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>

namespace daw::plugins
{

PluginHost::PluginHost() noexcept
{
}

PluginHost::~PluginHost()
{
    // Unload all plugins
    for (auto& plugin : plugins)
    {
        if (plugin && plugin->instance)
        {
            plugin->instance->releaseResources();
        }
    }
    plugins.clear();
}

std::unique_ptr<PluginHost::PluginInfo> PluginHost::loadPlugin(const std::string& pluginPath)
{
    if (pluginPath.empty())
        return nullptr;

    juce::AudioPluginFormatManager formatManager;
    formatManager.addDefaultFormats();

    juce::File pluginFile(pluginPath);
    if (!pluginFile.existsAsFile())
        return nullptr;

    juce::String errorMessage;
    std::unique_ptr<juce::AudioPluginInstance> instance;

    // Find a matching plugin description for this file
    juce::PluginDescription chosenDescription;
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        if (auto* format = formatManager.getFormat(i))
        {
            juce::OwnedArray<juce::PluginDescription> results;
            format->findAllTypesForFile(results, pluginFile.getFullPathName());

            if (!results.isEmpty())
            {
                chosenDescription = *results.getFirst();
                break;
            }
        }
    }

    if (chosenDescription.name.isEmpty())
        return nullptr;

    constexpr double defaultSampleRate = 44100.0;
    constexpr int defaultBlockSize = 512;

    instance = formatManager.createPluginInstance(
        chosenDescription,
        defaultSampleRate,
        defaultBlockSize,
        errorMessage);

    if (instance == nullptr || errorMessage.isNotEmpty())
        return nullptr;

    auto info = std::make_unique<PluginInfo>();
    info->name = instance->getName().toStdString();
    info->format = pluginFile.getFileExtension().toStdString();
    info->path = pluginPath;
    info->isLoaded = true;

    auto pluginInstance = std::make_unique<PluginInstance>();
    pluginInstance->info = std::move(info);
    pluginInstance->instance = std::move(instance);

    plugins.push_back(std::move(pluginInstance));

    return std::make_unique<PluginInfo>(*plugins.back()->info);
}

void PluginHost::unloadPlugin(const std::string& pluginId)
{
    auto it = std::remove_if(plugins.begin(), plugins.end(),
        [&pluginId](const std::unique_ptr<PluginInstance>& plugin)
        {
            return plugin && plugin->info && plugin->info->name == pluginId;
        });

    if (it != plugins.end())
    {
        if ((*it)->instance)
        {
            (*it)->instance->releaseResources();
        }
        plugins.erase(it, plugins.end());
    }
}

bool PluginHost::isPluginLoaded(const std::string& pluginId) const noexcept
{
    const auto* plugin = findPlugin(pluginId);
    return plugin != nullptr && plugin->info && plugin->info->isLoaded;
}

bool PluginHost::hasPluginCrashed(const std::string& pluginId) const noexcept
{
    const auto* plugin = findPlugin(pluginId);
    return plugin != nullptr && plugin->crashed.load(std::memory_order_acquire);
}

bool PluginHost::recoverFromCrash(const std::string& pluginId)
{
    auto* plugin = findPlugin(pluginId);
    if (plugin == nullptr || !plugin->crashed.load(std::memory_order_acquire))
        return false;

    // Mark as not crashed
    plugin->crashed.store(false, std::memory_order_release);

    // Attempt to reload plugin
    if (plugin->info)
    {
        const auto path = plugin->info->path;
        unloadPlugin(pluginId);
        return loadPlugin(path) != nullptr;
    }

    return false;
}

PluginHost::PluginInstance* PluginHost::findPlugin(const std::string& pluginId) noexcept
{
    auto it = std::find_if(plugins.begin(), plugins.end(),
        [&pluginId](const std::unique_ptr<PluginInstance>& plugin)
        {
            return plugin && plugin->info && plugin->info->name == pluginId;
        });

    return it != plugins.end() ? it->get() : nullptr;
}

const PluginHost::PluginInstance* PluginHost::findPlugin(const std::string& pluginId) const noexcept
{
    auto it = std::find_if(plugins.begin(), plugins.end(),
        [&pluginId](const std::unique_ptr<PluginInstance>& plugin)
        {
            return plugin && plugin->info && plugin->info->name == pluginId;
        });

    return it != plugins.end() ? it->get() : nullptr;
}

} // namespace daw::plugins
