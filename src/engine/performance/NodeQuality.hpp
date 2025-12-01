#pragma once
/**
 * @file NodeQuality.hpp
 * @brief Interface for DSP nodes with quality tier support.
 */

#include <chrono>
#include <cstdint>
#include <vector>

namespace cppmusic::engine::performance {

/**
 * @brief Quality tier for DSP processing.
 */
enum class QualityTier {
    Low,     ///< Minimal processing, lowest latency
    Medium,  ///< Balanced quality/performance
    High,    ///< Full quality, higher CPU usage
    Ultra    ///< Maximum quality, may exceed budget
};

/**
 * @brief Interface for nodes that support quality tier adjustment.
 * 
 * Nodes implementing this interface can adapt their processing
 * complexity based on available CPU budget.
 */
class NodeQuality {
public:
    virtual ~NodeQuality() = default;
    
    /**
     * @brief Get the list of supported quality tiers.
     */
    [[nodiscard]] virtual std::vector<QualityTier> getSupportedTiers() const = 0;
    
    /**
     * @brief Get the current quality tier.
     */
    [[nodiscard]] virtual QualityTier getCurrentTier() const = 0;
    
    /**
     * @brief Set the quality tier.
     * @param tier The desired quality tier.
     * 
     * If the requested tier is not supported, the nearest lower
     * supported tier will be used.
     */
    virtual void setQualityTier(QualityTier tier) = 0;
    
    /**
     * @brief Estimate CPU cost per sample at the given tier.
     * @param tier The quality tier to estimate.
     * @return Estimated cost in arbitrary units (higher = more CPU).
     * 
     * Used by PerformanceAdvisor to predict total block cost.
     */
    [[nodiscard]] virtual float estimateCostPerSample(QualityTier tier) const = 0;
    
    /**
     * @brief Get the name of this node for debugging/display.
     */
    [[nodiscard]] virtual const char* getNodeName() const = 0;
};

/**
 * @brief Convert quality tier to string.
 */
inline const char* toString(QualityTier tier) {
    switch (tier) {
        case QualityTier::Low: return "Low";
        case QualityTier::Medium: return "Medium";
        case QualityTier::High: return "High";
        case QualityTier::Ultra: return "Ultra";
    }
    return "Unknown";
}

/**
 * @brief Get the next lower quality tier.
 */
inline QualityTier decrementTier(QualityTier tier) {
    switch (tier) {
        case QualityTier::Ultra: return QualityTier::High;
        case QualityTier::High: return QualityTier::Medium;
        case QualityTier::Medium: return QualityTier::Low;
        case QualityTier::Low: return QualityTier::Low;
    }
    return QualityTier::Low;
}

/**
 * @brief Get the next higher quality tier.
 */
inline QualityTier incrementTier(QualityTier tier) {
    switch (tier) {
        case QualityTier::Low: return QualityTier::Medium;
        case QualityTier::Medium: return QualityTier::High;
        case QualityTier::High: return QualityTier::Ultra;
        case QualityTier::Ultra: return QualityTier::Ultra;
    }
    return QualityTier::Ultra;
}

} // namespace cppmusic::engine::performance
