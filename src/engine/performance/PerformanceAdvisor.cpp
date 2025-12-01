/**
 * @file PerformanceAdvisor.cpp
 * @brief Implementation of the adaptive performance advisor.
 */

#include "PerformanceAdvisor.hpp"
#include <algorithm>
#include <cmath>

namespace cppmusic::engine::performance {

PerformanceAdvisor::PerformanceAdvisor() = default;
PerformanceAdvisor::~PerformanceAdvisor() = default;

void PerformanceAdvisor::initialize(double sampleRate, std::size_t blockSize) {
    // Calculate block budget in microseconds
    // e.g., 512 samples at 48kHz = ~10.67ms
    double blockDurationSeconds = static_cast<double>(blockSize) / sampleRate;
    blockBudget_ = std::chrono::microseconds(
        static_cast<std::int64_t>(blockDurationSeconds * 1e6));
}

void PerformanceAdvisor::registerNode(NodeQuality* node) {
    if (!node) return;
    
    std::lock_guard<std::mutex> lock(nodesMutex_);
    auto it = std::find(nodes_.begin(), nodes_.end(), node);
    if (it == nodes_.end()) {
        nodes_.push_back(node);
        node->setQualityTier(currentTier_);
    }
}

void PerformanceAdvisor::unregisterNode(NodeQuality* node) {
    if (!node) return;
    
    std::lock_guard<std::mutex> lock(nodesMutex_);
    nodes_.erase(std::remove(nodes_.begin(), nodes_.end(), node), nodes_.end());
}

std::vector<NodeQuality*> PerformanceAdvisor::getNodes() const {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    return nodes_;
}

void PerformanceAdvisor::beginBlock() noexcept {
    blockStartTime_ = std::chrono::steady_clock::now();
}

void PerformanceAdvisor::endBlock() noexcept {
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - blockStartTime_);
    
    // Calculate load factor
    float load = 0.0f;
    if (blockBudget_.count() > 0) {
        load = static_cast<float>(duration.count()) / 
               static_cast<float>(blockBudget_.count());
    }
    
    recordLoadSample(load);
    
    // Check for critical load (>95% for 3 consecutive blocks)
    if (load > 0.95f) {
        int count = criticalCount_.fetch_add(1, std::memory_order_relaxed) + 1;
        if (count >= kCriticalThreshold && preferences_.allowAutoDowngrade) {
            triggerDowngrade(DowngradeReason::CriticalLoad);
            criticalCount_.store(0, std::memory_order_relaxed);
        }
    } else {
        criticalCount_.store(0, std::memory_order_relaxed);
    }
    
    // Check for sustained high load (>75% for 100 blocks)
    if (load > 0.75f) {
        int count = highLoadCount_.fetch_add(1, std::memory_order_relaxed) + 1;
        if (count >= kHighLoadThreshold && preferences_.allowAutoDowngrade) {
            triggerDowngrade(DowngradeReason::SustainedHighLoad);
            highLoadCount_.store(0, std::memory_order_relaxed);
        }
        lowLoadCount_.store(0, std::memory_order_relaxed);
    } else {
        highLoadCount_.store(0, std::memory_order_relaxed);
        
        // Check for sustained low load (<50% for 500 blocks)
        if (load < 0.50f) {
            int count = lowLoadCount_.fetch_add(1, std::memory_order_relaxed) + 1;
            if (count >= kLowLoadThreshold && preferences_.allowAutoUpgrade) {
                triggerUpgrade();
                lowLoadCount_.store(0, std::memory_order_relaxed);
            }
        } else {
            lowLoadCount_.store(0, std::memory_order_relaxed);
        }
    }
}

void PerformanceAdvisor::setGlobalTier(QualityTier tier) {
    currentTier_ = tier;
    applyTierToNodes();
    
    // Notify listeners
    std::lock_guard<std::mutex> lock(listenersMutex_);
    for (auto* listener : listeners_) {
        if (listener) {
            listener->onQualityUpgrade(tier, UpgradeReason::UserRequest);
        }
    }
}

QualityTier PerformanceAdvisor::getGlobalTier() const noexcept {
    return currentTier_;
}

void PerformanceAdvisor::setPreferences(const QualityPreferences& prefs) {
    preferences_ = prefs;
}

const QualityPreferences& PerformanceAdvisor::getPreferences() const noexcept {
    return preferences_;
}

float PerformanceAdvisor::getAverageLoad() const noexcept {
    return averageLoad_.load(std::memory_order_relaxed);
}

float PerformanceAdvisor::getPeakLoad() const noexcept {
    return peakLoad_.load(std::memory_order_relaxed);
}

std::vector<NodeLoadInfo> PerformanceAdvisor::getNodeLoads() const {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    std::vector<NodeLoadInfo> loads;
    loads.reserve(nodes_.size());
    
    for (const auto* node : nodes_) {
        NodeLoadInfo info;
        info.node = node;
        info.currentTier = node->getCurrentTier();
        info.estimatedCost = node->estimateCostPerSample(info.currentTier);
        loads.push_back(info);
    }
    
    return loads;
}

std::chrono::microseconds PerformanceAdvisor::getBlockBudget() const noexcept {
    return blockBudget_;
}

void PerformanceAdvisor::addListener(QualityChangeListener* listener) {
    if (!listener) return;
    
    std::lock_guard<std::mutex> lock(listenersMutex_);
    auto it = std::find(listeners_.begin(), listeners_.end(), listener);
    if (it == listeners_.end()) {
        listeners_.push_back(listener);
    }
}

void PerformanceAdvisor::removeListener(QualityChangeListener* listener) {
    if (!listener) return;
    
    std::lock_guard<std::mutex> lock(listenersMutex_);
    listeners_.erase(
        std::remove(listeners_.begin(), listeners_.end(), listener),
        listeners_.end());
}

void PerformanceAdvisor::triggerDowngrade(DowngradeReason reason) noexcept {
    // Don't go below minimum tier
    if (currentTier_ == preferences_.minimumTier) {
        return;
    }
    
    QualityTier newTier = decrementTier(currentTier_);
    if (newTier < preferences_.minimumTier) {
        newTier = preferences_.minimumTier;
    }
    
    currentTier_ = newTier;
    applyTierToNodes();
    
    // Notify listeners (note: this is called from audio thread context,
    // listeners should not perform blocking operations)
    std::lock_guard<std::mutex> lock(listenersMutex_);
    for (auto* listener : listeners_) {
        if (listener) {
            listener->onQualityDowngrade(newTier, reason);
        }
    }
}

void PerformanceAdvisor::triggerUpgrade() noexcept {
    // Don't go above preferred tier
    if (currentTier_ == preferences_.preferredTier) {
        return;
    }
    
    QualityTier newTier = incrementTier(currentTier_);
    if (newTier > preferences_.preferredTier) {
        newTier = preferences_.preferredTier;
    }
    
    currentTier_ = newTier;
    applyTierToNodes();
    
    // Notify listeners
    std::lock_guard<std::mutex> lock(listenersMutex_);
    for (auto* listener : listeners_) {
        if (listener) {
            listener->onQualityUpgrade(newTier, UpgradeReason::SustainedLowLoad);
        }
    }
}

void PerformanceAdvisor::applyTierToNodes() noexcept {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    for (auto* node : nodes_) {
        if (node) {
            node->setQualityTier(currentTier_);
        }
    }
}

void PerformanceAdvisor::recordLoadSample(float load) noexcept {
    // Update EMA
    emaLoad_ = kEmaAlpha * load + (1.0f - kEmaAlpha) * emaLoad_;
    averageLoad_.store(emaLoad_, std::memory_order_relaxed);
    
    // Update peak (decay slowly)
    float currentPeak = peakLoad_.load(std::memory_order_relaxed);
    if (load > currentPeak) {
        peakLoad_.store(load, std::memory_order_relaxed);
    } else {
        peakLoad_.store(currentPeak * 0.995f, std::memory_order_relaxed);
    }
}

} // namespace cppmusic::engine::performance
