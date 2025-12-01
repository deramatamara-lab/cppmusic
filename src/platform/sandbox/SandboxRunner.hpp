#pragma once
/**
 * @file SandboxRunner.hpp
 * @brief Sandbox process manager for plugin isolation.
 */

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace cppmusic::platform::sandbox {

/**
 * @brief Unique identifier for a sandbox instance.
 */
using SandboxId = std::uint64_t;

/**
 * @brief Invalid sandbox ID sentinel value.
 */
constexpr SandboxId InvalidSandboxId = 0;

/**
 * @brief Sandbox process status.
 */
enum class SandboxStatus {
    Starting,
    Running,
    Unresponsive,
    Crashed,
    Terminated
};

/**
 * @brief Configuration for sandbox creation.
 */
struct SandboxConfig {
    std::size_t maxMemoryMB = 512;
    std::chrono::milliseconds processTimeout{5000};
    std::vector<std::filesystem::path> allowedPaths;
    bool allowGPUAccess = false;
    bool autoRestart = true;
};

/**
 * @brief Plugin information for sandbox.
 */
struct PluginInfo {
    std::string name;
    std::filesystem::path path;
    std::string formatType;  // "VST3", "AU", "LV2", etc.
    std::string vendor;
};

/**
 * @brief Sandbox crash information.
 */
struct CrashInfo {
    SandboxId sandboxId = InvalidSandboxId;
    PluginInfo plugin;
    std::string reason;
    std::string reasonCode;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Listener for sandbox events.
 */
class SandboxListener {
public:
    virtual ~SandboxListener() = default;
    
    virtual void onSandboxStarted(SandboxId id, const PluginInfo& plugin) = 0;
    virtual void onSandboxCrash(const CrashInfo& info) = 0;
    virtual void onSandboxTerminated(SandboxId id) = 0;
};

/**
 * @brief Manages sandboxed plugin processes.
 * 
 * Provides process isolation for third-party plugins to prevent
 * crashes from affecting the main DAW process.
 * 
 * Current implementation: Stub for interface validation.
 * TODO: Implement actual process isolation using:
 * - Linux: fork() + seccomp + namespaces
 * - macOS: sandbox-exec or App Sandbox
 * - Windows: Job objects + restricted tokens
 */
class SandboxRunner {
public:
    SandboxRunner();
    ~SandboxRunner();
    
    // Non-copyable, non-movable
    SandboxRunner(const SandboxRunner&) = delete;
    SandboxRunner& operator=(const SandboxRunner&) = delete;
    SandboxRunner(SandboxRunner&&) = delete;
    SandboxRunner& operator=(SandboxRunner&&) = delete;
    
    // =========================================================================
    // Sandbox Management
    // =========================================================================
    
    /**
     * @brief Spawn a sandboxed plugin process.
     * @param plugin Plugin to run in sandbox.
     * @param config Sandbox configuration.
     * @return Sandbox ID, or InvalidSandboxId on failure.
     */
    SandboxId spawn(const PluginInfo& plugin, const SandboxConfig& config);
    
    /**
     * @brief Terminate a sandbox.
     */
    void terminate(SandboxId id);
    
    /**
     * @brief Terminate all sandboxes.
     */
    void terminateAll();
    
    /**
     * @brief Get sandbox status.
     */
    [[nodiscard]] SandboxStatus getStatus(SandboxId id) const;
    
    /**
     * @brief Get all active sandbox IDs.
     */
    [[nodiscard]] std::vector<SandboxId> getActiveSandboxes() const;
    
    /**
     * @brief Get plugin info for a sandbox.
     */
    [[nodiscard]] const PluginInfo* getPluginInfo(SandboxId id) const;
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Set watchdog timeout.
     */
    void setWatchdogTimeout(std::chrono::milliseconds timeout);
    
    /**
     * @brief Get current watchdog timeout.
     */
    [[nodiscard]] std::chrono::milliseconds getWatchdogTimeout() const noexcept;
    
    // =========================================================================
    // Event Listeners
    // =========================================================================
    
    /**
     * @brief Add sandbox event listener.
     */
    void addListener(SandboxListener* listener);
    
    /**
     * @brief Remove sandbox event listener.
     */
    void removeListener(SandboxListener* listener);
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

/**
 * @brief Get string representation of sandbox status.
 */
[[nodiscard]] const char* toString(SandboxStatus status);

} // namespace cppmusic::platform::sandbox
