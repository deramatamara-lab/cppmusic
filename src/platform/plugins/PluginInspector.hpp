#pragma once
/**
 * @file PluginInspector.hpp
 * @brief Plugin latency and health analysis.
 */

#include "../sandbox/SandboxRunner.hpp"
#include <chrono>
#include <cstdint>
#include <memory>

namespace cppmusic::platform::plugins {

// Forward declaration
namespace sandbox = cppmusic::platform::sandbox;

/**
 * @brief Plugin latency measurement report.
 */
struct PluginLatencyReport {
    std::chrono::microseconds reportedLatency{0};  ///< Plugin-reported latency
    std::chrono::microseconds measuredLatency{0};  ///< Actual measured latency
    std::chrono::microseconds jitter{0};           ///< Latency variation
    std::size_t samplesMeasured = 0;
};

/**
 * @brief Plugin resource usage metrics.
 */
struct PluginResourceUsage {
    float cpuPercent = 0.0f;
    std::size_t memoryMB = 0;
    std::size_t peakMemoryMB = 0;
    std::uint32_t audioDropouts = 0;
};

/**
 * @brief Plugin health status.
 */
enum class PluginHealthStatus {
    Healthy,
    Warning,
    Critical,
    Crashed
};

/**
 * @brief Latency difference significance.
 */
enum class LatencySignificance {
    None,      ///< < 5% change
    Minor,     ///< 5-15% change
    Major,     ///< > 15% change
    Critical   ///< Exceeds block budget
};

/**
 * @brief Difference between two latency measurements.
 */
struct LatencyDiff {
    std::chrono::microseconds baselineLatency{0};
    std::chrono::microseconds currentLatency{0};
    std::chrono::microseconds difference{0};
    float percentChange = 0.0f;
    LatencySignificance significance = LatencySignificance::None;
};

/**
 * @brief Analyzes plugin behavior and performance.
 */
class PluginInspector {
public:
    PluginInspector();
    ~PluginInspector();
    
    // Non-copyable, non-movable
    PluginInspector(const PluginInspector&) = delete;
    PluginInspector& operator=(const PluginInspector&) = delete;
    PluginInspector(PluginInspector&&) = delete;
    PluginInspector& operator=(PluginInspector&&) = delete;
    
    // =========================================================================
    // Latency Analysis
    // =========================================================================
    
    /**
     * @brief Measure plugin latency.
     * @param sandboxId Sandbox containing the plugin.
     */
    [[nodiscard]] PluginLatencyReport measureLatency(sandbox::SandboxId sandboxId);
    
    /**
     * @brief Compute latency difference.
     */
    [[nodiscard]] LatencyDiff computeLatencyDiff(
        const PluginLatencyReport& before,
        const PluginLatencyReport& after) const;
    
    // =========================================================================
    // Resource Monitoring
    // =========================================================================
    
    /**
     * @brief Get resource usage for a plugin.
     */
    [[nodiscard]] PluginResourceUsage getResourceUsage(sandbox::SandboxId sandboxId);
    
    /**
     * @brief Get overall plugin health status.
     */
    [[nodiscard]] PluginHealthStatus getHealthStatus(sandbox::SandboxId sandboxId);
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Set the block budget for critical latency determination.
     */
    void setBlockBudget(std::chrono::microseconds budget);
    
    /**
     * @brief Get current block budget.
     */
    [[nodiscard]] std::chrono::microseconds getBlockBudget() const noexcept;
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

/**
 * @brief Get string representation of health status.
 */
[[nodiscard]] const char* toString(PluginHealthStatus status);

/**
 * @brief Get string representation of latency significance.
 */
[[nodiscard]] const char* toString(LatencySignificance significance);

} // namespace cppmusic::platform::plugins
