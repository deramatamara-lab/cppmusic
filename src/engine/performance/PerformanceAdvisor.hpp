#pragma once
/**
 * @file PerformanceAdvisor.hpp
 * @brief Adaptive performance management with quality tier negotiation.
 */

#include "NodeQuality.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <vector>

namespace cppmusic::engine::performance {

/**
 * @brief Reason for quality downgrade.
 */
enum class DowngradeReason {
    SustainedHighLoad,  ///< Load > 75% for extended period
    CriticalLoad,       ///< Load > 95% detected
    AudioDropout,       ///< Buffer underrun occurred
    UserRequest         ///< User manually requested
};

/**
 * @brief Reason for quality upgrade.
 */
enum class UpgradeReason {
    SustainedLowLoad,   ///< Load < 50% for extended period
    UserRequest         ///< User manually requested
};

/**
 * @brief User-configurable quality preferences.
 */
struct QualityPreferences {
    QualityTier preferredTier = QualityTier::High;
    QualityTier minimumTier = QualityTier::Medium;
    bool allowAutoDowngrade = true;
    bool allowAutoUpgrade = true;
    float targetLoadPercent = 75.0f;
};

/**
 * @brief Load information for a single node.
 */
struct NodeLoadInfo {
    const NodeQuality* node = nullptr;
    QualityTier currentTier = QualityTier::Medium;
    float estimatedCost = 0.0f;
    float measuredDuration = 0.0f;
};

/**
 * @brief Block processing metrics.
 */
struct BlockMetrics {
    std::chrono::microseconds totalDuration{0};
    std::chrono::microseconds budgetMicros{0};
    float loadFactor = 0.0f;
    
    [[nodiscard]] bool isOverloaded() const noexcept {
        return loadFactor > 0.75f;
    }
    
    [[nodiscard]] bool isCritical() const noexcept {
        return loadFactor > 0.95f;
    }
};

/**
 * @brief Listener for quality change events.
 */
class QualityChangeListener {
public:
    virtual ~QualityChangeListener() = default;
    virtual void onQualityDowngrade(QualityTier newTier, DowngradeReason reason) = 0;
    virtual void onQualityUpgrade(QualityTier newTier, UpgradeReason reason) = 0;
};

/**
 * @brief Manages adaptive performance through quality tier adjustment.
 * 
 * Monitors CPU load and adjusts node quality tiers to maintain
 * smooth audio processing within budget.
 */
class PerformanceAdvisor {
public:
    PerformanceAdvisor();
    ~PerformanceAdvisor();
    
    // Non-copyable, non-movable
    PerformanceAdvisor(const PerformanceAdvisor&) = delete;
    PerformanceAdvisor& operator=(const PerformanceAdvisor&) = delete;
    PerformanceAdvisor(PerformanceAdvisor&&) = delete;
    PerformanceAdvisor& operator=(PerformanceAdvisor&&) = delete;
    
    // =========================================================================
    // Initialization
    // =========================================================================
    
    /**
     * @brief Initialize with audio settings.
     * @param sampleRate Audio sample rate in Hz.
     * @param blockSize Audio block size in samples.
     */
    void initialize(double sampleRate, std::size_t blockSize);
    
    // =========================================================================
    // Node Management
    // =========================================================================
    
    /**
     * @brief Register a node for quality management.
     */
    void registerNode(NodeQuality* node);
    
    /**
     * @brief Unregister a node.
     */
    void unregisterNode(NodeQuality* node);
    
    /**
     * @brief Get all registered nodes.
     */
    [[nodiscard]] std::vector<NodeQuality*> getNodes() const;
    
    // =========================================================================
    // Block Processing (call from audio thread)
    // =========================================================================
    
    /**
     * @brief Mark the beginning of a block processing cycle.
     * 
     * Call this at the start of each audio block before processing.
     */
    void beginBlock() noexcept;
    
    /**
     * @brief Mark the end of a block processing cycle.
     * 
     * Call this after all audio processing is complete.
     * This triggers load measurement and potential tier adjustment.
     */
    void endBlock() noexcept;
    
    // =========================================================================
    // Quality Control
    // =========================================================================
    
    /**
     * @brief Set the global quality tier for all nodes.
     */
    void setGlobalTier(QualityTier tier);
    
    /**
     * @brief Get the current global quality tier.
     */
    [[nodiscard]] QualityTier getGlobalTier() const noexcept;
    
    /**
     * @brief Set quality preferences.
     */
    void setPreferences(const QualityPreferences& prefs);
    
    /**
     * @brief Get current preferences.
     */
    [[nodiscard]] const QualityPreferences& getPreferences() const noexcept;
    
    // =========================================================================
    // Statistics
    // =========================================================================
    
    /**
     * @brief Get the average CPU load (0.0 - 1.0).
     */
    [[nodiscard]] float getAverageLoad() const noexcept;
    
    /**
     * @brief Get the peak CPU load (0.0 - 1.0).
     */
    [[nodiscard]] float getPeakLoad() const noexcept;
    
    /**
     * @brief Get load information for each node.
     */
    [[nodiscard]] std::vector<NodeLoadInfo> getNodeLoads() const;
    
    /**
     * @brief Get the block budget in microseconds.
     */
    [[nodiscard]] std::chrono::microseconds getBlockBudget() const noexcept;
    
    // =========================================================================
    // Event Listeners
    // =========================================================================
    
    /**
     * @brief Add a listener for quality change events.
     */
    void addListener(QualityChangeListener* listener);
    
    /**
     * @brief Remove a listener.
     */
    void removeListener(QualityChangeListener* listener);
    
private:
    /**
     * @brief Trigger a quality downgrade.
     */
    void triggerDowngrade(DowngradeReason reason) noexcept;
    
    /**
     * @brief Trigger a quality upgrade.
     */
    void triggerUpgrade() noexcept;
    
    /**
     * @brief Apply the current tier to all nodes.
     */
    void applyTierToNodes() noexcept;
    
    /**
     * @brief Record a load sample for averaging.
     */
    void recordLoadSample(float load) noexcept;
    
    std::vector<NodeQuality*> nodes_;
    std::vector<QualityChangeListener*> listeners_;
    mutable std::mutex nodesMutex_;
    mutable std::mutex listenersMutex_;
    
    QualityPreferences preferences_;
    QualityTier currentTier_ = QualityTier::High;
    
    std::chrono::microseconds blockBudget_{0};
    std::chrono::steady_clock::time_point blockStartTime_;
    
    // Load tracking
    std::atomic<float> averageLoad_{0.0f};
    std::atomic<float> peakLoad_{0.0f};
    float emaLoad_ = 0.0f;
    static constexpr float kEmaAlpha = 0.1f;
    
    // Trigger counters
    std::atomic<int> highLoadCount_{0};
    std::atomic<int> criticalCount_{0};
    std::atomic<int> lowLoadCount_{0};
    
    // Thresholds (in number of blocks)
    static constexpr int kHighLoadThreshold = 100;
    static constexpr int kCriticalThreshold = 3;
    static constexpr int kLowLoadThreshold = 500;
};

} // namespace cppmusic::engine::performance
