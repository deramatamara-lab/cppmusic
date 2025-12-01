/**
 * @file OffloadManager.cpp
 * @brief Implementation of the CPU/GPU offload heuristic scheduler
 */

#include "OffloadManager.hpp"
#include "GpuContext.hpp"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <unordered_map>

namespace cppmusic::dsp::offload {

struct OperationHistory {
    uint64_t cpuExecutions{0};
    uint64_t gpuExecutions{0};
    std::chrono::microseconds avgCpuLatency{0};
    std::chrono::microseconds avgGpuLatency{0};
    uint64_t deadlineMisses{0};
};

struct OffloadManager::Impl {
    std::shared_ptr<GpuContext> gpuContext;
    OffloadConfig config;
    OffloadStats stats;
    double currentCpuLoad{0.0};
    
    mutable std::mutex mutex;
    std::unordered_map<std::string, OperationHistory> history;
    
    void updateAvgLatency(std::chrono::microseconds& avg, 
                          std::chrono::microseconds newValue,
                          uint64_t count) {
        // Exponential moving average
        if (count <= 1) {
            avg = newValue;
        } else {
            constexpr double alpha = 0.1;
            auto avgUs = static_cast<double>(avg.count());
            auto newUs = static_cast<double>(newValue.count());
            avg = std::chrono::microseconds(
                static_cast<int64_t>(avgUs * (1.0 - alpha) + newUs * alpha));
        }
    }
};

OffloadManager::OffloadManager(std::shared_ptr<GpuContext> gpuContext)
    : impl_(std::make_unique<Impl>()) {
    impl_->gpuContext = std::move(gpuContext);
}

OffloadManager::~OffloadManager() = default;

void OffloadManager::setGpuContext(std::shared_ptr<GpuContext> gpuContext) {
    std::lock_guard lock(impl_->mutex);
    impl_->gpuContext = std::move(gpuContext);
}

const OffloadConfig& OffloadManager::getConfig() const {
    return impl_->config;
}

void OffloadManager::setConfig(const OffloadConfig& config) {
    std::lock_guard lock(impl_->mutex);
    impl_->config = config;
}

OffloadDecision OffloadManager::decide(const OffloadCandidate& candidate) {
    std::lock_guard lock(impl_->mutex);
    
    // If GPU not available, always use CPU
    if (!impl_->gpuContext || !impl_->gpuContext->isAvailable()) {
        return OffloadDecision::UseCpu;
    }
    
    const auto& config = impl_->config;
    
    // Check minimum size threshold
    size_t totalSize = candidate.inputSizeBytes + candidate.outputSizeBytes;
    if (totalSize < config.minOffloadSize) {
        return OffloadDecision::UseCpu;
    }
    
    // Estimate transfer latency
    auto transferLatency = std::chrono::microseconds(
        impl_->gpuContext->estimateTransferLatencyUs(totalSize));
    
    if (transferLatency > config.maxTransferLatency) {
        return OffloadDecision::UseCpu;
    }
    
    // Calculate effective GPU time (compute + transfer)
    auto effectiveGpuTime = candidate.gpuEstimate + transferLatency;
    
    // Check deadline constraints
    if (candidate.deadline.count() > 0) {
        auto safeDeadline = std::chrono::microseconds(
            static_cast<int64_t>(candidate.deadline.count() * 
                                  (1.0 - config.deadlineMargin)));
        
        // If CPU can meet deadline safely, prefer it for simplicity
        if (candidate.cpuEstimate <= safeDeadline) {
            // But check if GPU offers significant improvement
            double speedup = static_cast<double>(candidate.cpuEstimate.count()) /
                           static_cast<double>(effectiveGpuTime.count());
            
            if (speedup >= config.minGpuSpeedup) {
                return OffloadDecision::UseGpu;
            }
            
            // Also consider CPU load
            if (impl_->currentCpuLoad >= config.cpuLoadThreshold &&
                effectiveGpuTime <= safeDeadline) {
                return OffloadDecision::UseGpu;
            }
            
            return OffloadDecision::UseCpu;
        }
        
        // CPU cannot meet deadline, check GPU
        if (effectiveGpuTime <= safeDeadline) {
            return OffloadDecision::UseGpu;
        }
        
        // Neither can meet deadline - try async GPU for next frame
        return OffloadDecision::UseGpuAsync;
    }
    
    // No deadline constraint - use speedup heuristic
    double speedup = static_cast<double>(candidate.cpuEstimate.count()) /
                     static_cast<double>(effectiveGpuTime.count());
    
    if (speedup >= config.minGpuSpeedup) {
        return OffloadDecision::UseGpu;
    }
    
    // Consider CPU load for marginal cases
    if (impl_->currentCpuLoad >= config.cpuLoadThreshold && speedup >= 1.0) {
        return OffloadDecision::UseGpu;
    }
    
    return OffloadDecision::UseCpu;
}

std::vector<OffloadDecision> OffloadManager::decideBatch(
    const std::vector<OffloadCandidate>& candidates) {
    
    std::vector<OffloadDecision> decisions;
    decisions.reserve(candidates.size());
    
    // For now, decide individually
    // TODO: Implement batch optimization (group similar GPU operations)
    for (const auto& candidate : candidates) {
        decisions.push_back(decide(candidate));
    }
    
    return decisions;
}

void OffloadManager::reportExecution(const std::string& operationId,
                                      OffloadDecision decision,
                                      std::chrono::microseconds actualLatency) {
    std::lock_guard lock(impl_->mutex);
    
    auto& hist = impl_->history[operationId];
    auto& stats = impl_->stats;
    
    switch (decision) {
        case OffloadDecision::UseCpu:
            hist.cpuExecutions++;
            stats.cpuExecutions++;
            impl_->updateAvgLatency(hist.avgCpuLatency, actualLatency, 
                                    hist.cpuExecutions);
            impl_->updateAvgLatency(stats.avgCpuLatency, actualLatency,
                                    stats.cpuExecutions);
            break;
            
        case OffloadDecision::UseGpu:
        case OffloadDecision::UseGpuAsync:
            hist.gpuExecutions++;
            stats.gpuExecutions++;
            impl_->updateAvgLatency(hist.avgGpuLatency, actualLatency,
                                    hist.gpuExecutions);
            impl_->updateAvgLatency(stats.avgGpuLatency, actualLatency,
                                    stats.gpuExecutions);
            break;
            
        case OffloadDecision::Defer:
            stats.deferredExecutions++;
            break;
    }
    
    // Update CPU savings estimate
    if (decision == OffloadDecision::UseGpu || 
        decision == OffloadDecision::UseGpuAsync) {
        if (hist.avgCpuLatency.count() > 0) {
            stats.cpuSavings += static_cast<double>(
                hist.avgCpuLatency.count() - actualLatency.count());
        }
    }
}

void OffloadManager::reportDeadlineMiss(const std::string& operationId,
                                         OffloadDecision decision,
                                         std::chrono::microseconds actualLatency) {
    std::lock_guard lock(impl_->mutex);
    
    impl_->history[operationId].deadlineMisses++;
    impl_->stats.deadlineMisses++;
    
    // Report as regular execution too
    reportExecution(operationId, decision, actualLatency);
}

OffloadStats OffloadManager::getStats() const {
    std::lock_guard lock(impl_->mutex);
    return impl_->stats;
}

void OffloadManager::resetStats() {
    std::lock_guard lock(impl_->mutex);
    impl_->stats = OffloadStats{};
    impl_->history.clear();
}

void OffloadManager::updateCpuLoad(double load) {
    std::lock_guard lock(impl_->mutex);
    impl_->currentCpuLoad = std::clamp(load, 0.0, 1.0);
}

bool OffloadManager::isGpuAvailable() const {
    std::lock_guard lock(impl_->mutex);
    return impl_->gpuContext && impl_->gpuContext->isAvailable();
}

std::chrono::microseconds OffloadManager::getLatencyBudget(
    size_t blockSize, double sampleRate) const {
    
    // Calculate block duration
    double blockDurationUs = (static_cast<double>(blockSize) / sampleRate) * 1e6;
    
    // Reserve margin for safety (25% of block time for GPU operations)
    double budgetUs = blockDurationUs * 0.25;
    
    // Apply minimum and maximum bounds
    constexpr double minBudgetUs = 100.0;
    constexpr double maxBudgetUs = 10000.0;
    
    budgetUs = std::clamp(budgetUs, minBudgetUs, maxBudgetUs);
    
    return std::chrono::microseconds(static_cast<int64_t>(budgetUs));
}

}  // namespace cppmusic::dsp::offload
