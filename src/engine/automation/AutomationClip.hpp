#pragma once
/**
 * @file AutomationClip.hpp
 * @brief Automation curve with hierarchical layer system and deterministic evaluation.
 */

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cppmusic::engine::automation {

/**
 * @brief Interpolation curve type between automation points.
 */
enum class CurveType {
    Step,    ///< Instant change at breakpoint
    Linear,  ///< Straight line between points
    Bezier,  ///< Smooth cubic bezier
    SCurve   ///< Smooth step (ease in/out)
};

/**
 * @brief Bezier control handles for smooth curves.
 */
struct BezierHandles {
    float outTangentX = 0.25f;
    float outTangentY = 0.0f;
    float inTangentX = 0.75f;
    float inTangentY = 0.0f;
};

/**
 * @brief A single automation breakpoint.
 */
struct AutomationPoint {
    double beat = 0.0;
    float value = 0.0f;
    CurveType curveToNext = CurveType::Linear;
    std::optional<BezierHandles> handles;
    
    bool operator<(const AutomationPoint& other) const noexcept {
        return beat < other.beat;
    }
};

/**
 * @brief Layer type for hierarchical automation.
 */
enum class LayerType {
    Base,     ///< Foundational automation curve
    Override, ///< Temporary override for specific regions
    Macro     ///< Global modifier affecting all automation
};

/**
 * @brief Override layer with fade in/out regions.
 */
struct OverrideRegion {
    double startBeat = 0.0;
    double endBeat = 0.0;
    double fadeInBeats = 0.25;
    double fadeOutBeats = 0.25;
    std::vector<AutomationPoint> points;
    
    /**
     * @brief Check if this region is active at the given beat.
     */
    [[nodiscard]] bool isActiveAt(double beat) const noexcept {
        return beat >= startBeat && beat <= endBeat;
    }
    
    /**
     * @brief Get the blend factor at the given beat (0-1).
     */
    [[nodiscard]] float getBlendFactor(double beat) const noexcept;
    
    /**
     * @brief Evaluate the override curve at the given beat.
     */
    [[nodiscard]] float evaluate(double beat) const noexcept;
};

/**
 * @brief Macro layer transformation.
 */
struct MacroTransform {
    float scale = 1.0f;
    float offset = 0.0f;
    bool inverted = false;
    
    /**
     * @brief Apply transformation to a value.
     */
    [[nodiscard]] float transform(float value) const noexcept {
        float result = value * scale + offset;
        if (inverted) result = 1.0f - result;
        return result;
    }
};

/**
 * @brief Automation clip with hierarchical layer system.
 * 
 * Evaluation order:
 * 1. Base layer provides foundational curve
 * 2. Override layers blend on top of base
 * 3. Macro transformation applied last
 */
class AutomationClip {
public:
    AutomationClip();
    ~AutomationClip();
    
    // Copyable and movable for versioning
    AutomationClip(const AutomationClip& other);
    AutomationClip& operator=(const AutomationClip& other);
    AutomationClip(AutomationClip&&) noexcept = default;
    AutomationClip& operator=(AutomationClip&&) noexcept = default;
    
    // =========================================================================
    // Base Layer Management
    // =========================================================================
    
    /**
     * @brief Add a point to the base automation curve.
     */
    void addPoint(const AutomationPoint& point);
    
    /**
     * @brief Remove a point by index.
     */
    bool removePoint(std::size_t index);
    
    /**
     * @brief Clear all base layer points.
     */
    void clearPoints();
    
    /**
     * @brief Get all base layer points.
     */
    [[nodiscard]] const std::vector<AutomationPoint>& getPoints() const noexcept;
    
    /**
     * @brief Get the number of base layer points.
     */
    [[nodiscard]] std::size_t getPointCount() const noexcept;
    
    // =========================================================================
    // Override Layer Management
    // =========================================================================
    
    /**
     * @brief Add an override region.
     */
    void addOverride(const OverrideRegion& region);
    
    /**
     * @brief Remove an override by index.
     */
    bool removeOverride(std::size_t index);
    
    /**
     * @brief Clear all override regions.
     */
    void clearOverrides();
    
    /**
     * @brief Get all override regions.
     */
    [[nodiscard]] const std::vector<OverrideRegion>& getOverrides() const noexcept;
    
    // =========================================================================
    // Macro Layer Management
    // =========================================================================
    
    /**
     * @brief Set the macro transformation.
     */
    void setMacroTransform(const MacroTransform& transform);
    
    /**
     * @brief Get the current macro transformation.
     */
    [[nodiscard]] const MacroTransform& getMacroTransform() const noexcept;
    
    /**
     * @brief Enable or disable macro transformation.
     */
    void setMacroEnabled(bool enabled);
    
    /**
     * @brief Check if macro transformation is enabled.
     */
    [[nodiscard]] bool isMacroEnabled() const noexcept;
    
    // =========================================================================
    // Evaluation
    // =========================================================================
    
    /**
     * @brief Evaluate the automation curve at the given beat.
     * 
     * Processes all layers in order:
     * 1. Base layer
     * 2. Override layers (blended)
     * 3. Macro transformation
     * 
     * @param beat The beat position to evaluate.
     * @return The automation value (clamped to 0-1).
     */
    [[nodiscard]] float evaluate(double beat) const noexcept;
    
    /**
     * @brief Evaluate only the base layer (no overrides or macro).
     */
    [[nodiscard]] float evaluateBase(double beat) const noexcept;
    
    // =========================================================================
    // Serialization
    // =========================================================================
    
    /**
     * @brief Serialize the clip to binary data.
     */
    [[nodiscard]] std::vector<std::uint8_t> serialize() const;
    
    /**
     * @brief Deserialize from binary data.
     */
    static AutomationClip deserialize(const std::vector<std::uint8_t>& data);
    
    /**
     * @brief Compute a deterministic hash of the clip state.
     */
    [[nodiscard]] std::uint64_t computeHash() const noexcept;
    
private:
    /**
     * @brief Interpolate between two points.
     */
    [[nodiscard]] float interpolate(const AutomationPoint& p1,
                                    const AutomationPoint& p2,
                                    double beat) const noexcept;
    
    std::vector<AutomationPoint> basePoints_;
    std::vector<OverrideRegion> overrides_;
    MacroTransform macroTransform_;
    bool macroEnabled_ = false;
};

} // namespace cppmusic::engine::automation
