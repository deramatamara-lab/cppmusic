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
    juce::ignoreUnused(pluginPath);
    
    // TODO: Implement actual plugin loading
    // This would use juce::VST3PluginFormat, juce::AudioUnitPluginFormat, etc.
    // For now, return nullptr as placeholder
    
    return nullptr;
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

