#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace daw::plugins
{

/**
 * @brief Plugin capability database
 * 
 * Stores plugin metadata: channels, MIDI support, latency, sidechain support.
 * Scans and caches plugin information for efficient loading.
 */
class PluginDatabase
{
public:
    struct PluginCapabilities
    {
        std::string pluginId;
        std::string name;
        std::string format;
        std::string path;
        
        int maxInputChannels{2};
        int maxOutputChannels{2};
        bool supportsMIDI{false};
        int latencySamples{0};
        bool supportsSidechain{false};
        bool isStable{true};
        
        std::string version;
    };

    PluginDatabase() noexcept = default;
    ~PluginDatabase() = default;

    // Non-copyable, movable
    PluginDatabase(const PluginDatabase&) = delete;
    PluginDatabase& operator=(const PluginDatabase&) = delete;
    PluginDatabase(PluginDatabase&&) noexcept = default;
    PluginDatabase& operator=(PluginDatabase&&) noexcept = default;

    /**
     * @brief Scan plugin directory and build database
     * @param pluginDirectory Path to plugin directory
     */
    void scanPluginDirectory(const std::string& pluginDirectory);

    /**
     * @brief Get plugin capabilities
     * @param pluginId Plugin identifier
     * @return Plugin capabilities, or nullptr if not found
     */
    [[nodiscard]] const PluginCapabilities* getCapabilities(const std::string& pluginId) const noexcept;

    /**
     * @brief Add or update plugin capabilities
     */
    void setCapabilities(const PluginCapabilities& capabilities);

    /**
     * @brief Get all known plugins
     */
    [[nodiscard]] std::vector<std::string> getAllPluginIds() const noexcept;

    /**
     * @brief Clear database
     */
    void clear() noexcept;

private:
    std::unordered_map<std::string, PluginCapabilities> database;
};

} // namespace daw::plugins

