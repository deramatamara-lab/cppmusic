#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <string>
#include <vector>
#include <atomic>

namespace daw::plugins
{

/**
 * @brief Process-isolated plugin hosting
 * 
 * Hosts plugins in separate processes for crash isolation.
 * Uses shared-memory IPC for zero-copy audio communication.
 * Follows DAW_DEV_RULES: crash recovery, autosave.
 */
class PluginHost
{
public:
    struct PluginInfo
    {
        std::string name;
        std::string format; // VST3, AU, AAX
        std::string path;
        bool isLoaded{false};
        bool isCrashed{false};
    };

    PluginHost() noexcept;
    ~PluginHost();

    // Non-copyable, movable
    PluginHost(const PluginHost&) = delete;
    PluginHost& operator=(const PluginHost&) = delete;
    PluginHost(PluginHost&&) noexcept = default;
    PluginHost& operator=(PluginHost&&) noexcept = default;

    /**
     * @brief Load plugin from path
     * @param pluginPath Path to plugin file
     * @return Plugin info if successful, nullptr otherwise
     */
    std::unique_ptr<PluginInfo> loadPlugin(const std::string& pluginPath);

    /**
     * @brief Unload plugin
     * @param pluginId Plugin identifier
     */
    void unloadPlugin(const std::string& pluginId);

    /**
     * @brief Check if plugin is loaded
     */
    [[nodiscard]] bool isPluginLoaded(const std::string& pluginId) const noexcept;

    /**
     * @brief Check if plugin has crashed
     */
    [[nodiscard]] bool hasPluginCrashed(const std::string& pluginId) const noexcept;

    /**
     * @brief Recover from plugin crash
     * @param pluginId Plugin identifier
     * @return true if recovery successful
     */
    bool recoverFromCrash(const std::string& pluginId);

private:
    struct PluginInstance
    {
        std::unique_ptr<PluginInfo> info;
        std::unique_ptr<juce::AudioPluginInstance> instance;
        std::atomic<bool> crashed{false};
    };

    std::vector<std::unique_ptr<PluginInstance>> plugins;
    
    [[nodiscard]] PluginInstance* findPlugin(const std::string& pluginId) noexcept;
    [[nodiscard]] const PluginInstance* findPlugin(const std::string& pluginId) const noexcept;
};

} // namespace daw::plugins

