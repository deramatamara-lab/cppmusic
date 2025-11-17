#include "PluginDatabase.h"
#include <algorithm>

namespace daw::plugins
{

void PluginDatabase::scanPluginDirectory(const std::string& pluginDirectory)
{
    static_cast<void>(pluginDirectory);
    
    // TODO: Implement actual plugin scanning
    // This would use juce::KnownPluginList and format managers
    // to scan VST3, AU, AAX plugins and extract capabilities
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

