/**
 * @file OffloadManager.hpp
 * @brief Heuristic scheduler for CPU/GPU DSP offloading decisions
 *
 * The OffloadManager determines when to offload DSP operations to the GPU
 * based on workload characteristics, latency requirements, and resource
 * availability.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cppmusic::dsp::offload {

class GpuContext;

/**
 * @brief Describes a potential offload operation
 */
struct OffloadCandidate {
    std::string operationId;
    
    // Workload characteristics
    size_t inputSizeBytes{0};
    size_t outputSizeBytes{0};
    uint64_t estimatedCpuCyclesCost{0};
    uint64_t estimatedGpuCyclesCost{0};
    
    // Timing requirements
    std::chrono::microseconds deadline{0};
    std::chrono::microseconds cpuEstimate{0};
    std::chrono::microseconds gpuEstimate{0};  // Including transfer
    
    // Dependencies
    bool requiresGpuMemoryResident{false};
    bool canBeBatched{false};
};

/**
 * @brief Decision from the offload manager
 */
enum class OffloadDecision {
    UseCpu,           // Execute on CPU
    UseGpu,           // Offload to GPU
    UseGpuAsync,      // Offload to GPU asynchronously (for next frame)
    Defer             // Defer decision (gather more candidates for batching)
};

/**
 * @brief Statistics for monitoring offload performance
 */
struct OffloadStats {
    uint64_t cpuExecutions{0};
    uint64_t gpuExecutions{0};
    uint64_t deferredExecutions{0};
    uint64_t deadlineMisses{0};
    
    std::chrono::microseconds avgCpuLatency{0};
    std::chrono::microseconds avgGpuLatency{0};
    std::chrono::microseconds avgTransferOverhead{0};
    
    double gpuUtilization{0.0};
    double cpuSavings{0.0};  // Estimated CPU time saved by GPU offload
};

/**
 * @brief Configuration for the offload manager
 */
struct OffloadConfig {
    // Minimum workload size to consider GPU offload (bytes)
    size_t minOffloadSize{1024};
    
    // Maximum transfer latency acceptable (microseconds)
    std::chrono::microseconds maxTransferLatency{500};
    
    // Deadline safety margin (percentage, 0.0-1.0)
    double deadlineMargin{0.2};
    
    // Enable batching of similar operations
    bool enableBatching{true};
    
    // Maximum batch wait time
    std::chrono::microseconds maxBatchWait{100};
    
    // Prefer GPU when CPU load exceeds this threshold (0.0-1.0)
    double cpuLoadThreshold{0.7};
    
    // Minimum GPU speedup required (ratio)
    double minGpuSpeedup{2.0};
};

/**
 * @brief Manages CPU/GPU DSP offloading decisions
 *
 * The OffloadManager uses heuristics to determine the optimal execution
 * target for DSP operations. It considers:
 * - Workload size and complexity
 * - Transfer overhead vs. compute savings
 * - Real-time deadline requirements
 * - Current CPU/GPU load
 * - Batching opportunities
 */
class OffloadManager {
public:
    /**
     * @brief Construct with optional GPU context
     * @param gpuContext GPU context (may be null if GPU unavailable)
     */
    explicit OffloadManager(std::shared_ptr<GpuContext> gpuContext = nullptr);
    ~OffloadManager();

    // Non-copyable
    OffloadManager(const OffloadManager&) = delete;
    OffloadManager& operator=(const OffloadManager&) = delete;

    /**
     * @brief Set the GPU context
     */
    void setGpuContext(std::shared_ptr<GpuContext> gpuContext);

    /**
     * @brief Get current configuration
     */
    [[nodiscard]] const OffloadConfig& getConfig() const;

    /**
     * @brief Update configuration
     */
    void setConfig(const OffloadConfig& config);

    /**
     * @brief Decide whether to offload an operation
     * @param candidate Description of the operation
     * @return Offload decision
     */
    [[nodiscard]] OffloadDecision decide(const OffloadCandidate& candidate);

    /**
     * @brief Submit multiple candidates for batch decision
     * @param candidates List of candidates
     * @return Decisions for each candidate (same order)
     */
    [[nodiscard]] std::vector<OffloadDecision> decideBatch(
        const std::vector<OffloadCandidate>& candidates);

    /**
     * @brief Report actual execution metrics for learning
     * @param operationId Operation identifier
     * @param decision Decision that was made
     * @param actualLatency Actual execution time
     */
    void reportExecution(const std::string& operationId,
                         OffloadDecision decision,
                         std::chrono::microseconds actualLatency);

    /**
     * @brief Report a deadline miss
     * @param operationId Operation identifier
     * @param decision Decision that was made
     * @param actualLatency Actual execution time
     */
    void reportDeadlineMiss(const std::string& operationId,
                            OffloadDecision decision,
                            std::chrono::microseconds actualLatency);

    /**
     * @brief Get current statistics
     */
    [[nodiscard]] OffloadStats getStats() const;

    /**
     * @brief Reset statistics
     */
    void resetStats();

    /**
     * @brief Update CPU load estimate
     * @param load Current CPU load (0.0-1.0)
     */
    void updateCpuLoad(double load);

    /**
     * @brief Check if GPU offloading is available
     */
    [[nodiscard]] bool isGpuAvailable() const;

    /**
     * @brief Get recommended latency budget for GPU operations
     * @param blockSize Audio block size in samples
     * @param sampleRate Sample rate in Hz
     * @return Recommended maximum GPU operation latency
     */
    [[nodiscard]] std::chrono::microseconds getLatencyBudget(
        size_t blockSize, double sampleRate) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace cppmusic::dsp::offload
