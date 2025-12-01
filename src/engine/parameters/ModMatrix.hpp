#pragma once
/**
 * @file ModMatrix.hpp
 * @brief Modulation matrix for routing modulation sources to parameter targets.
 */

#include "ParamSignal.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace cppmusic::engine::parameters {

// Forward declaration
class ParamRegistry;

/**
 * @brief Unique identifier for modulation slots.
 */
using ModSlotId = std::uint32_t;

/**
 * @brief Invalid mod slot ID sentinel value.
 */
constexpr ModSlotId InvalidModSlotId = 0;

/**
 * @brief Modulation blending mode.
 */
enum class BlendMode {
    Add,      ///< target += mod * amount
    Multiply, ///< target *= (1 + mod * amount)
    Replace,  ///< target = mod * amount
    Bipolar   ///< target += (mod - 0.5) * 2 * amount
};

/**
 * @brief Represents a modulation source.
 */
struct ModSource {
    enum class Type {
        Parameter,  ///< Another parameter
        LFO,        ///< LFO signal
        Envelope,   ///< Envelope signal
        External    ///< External control (MIDI, etc.)
    };
    
    Type type = Type::Parameter;
    ParamId paramId = InvalidParamId;  // If type is Parameter
    std::uint32_t sourceIndex = 0;      // For LFO/Envelope/External
    
    /**
     * @brief Get the current value of this modulation source.
     * @param registry The parameter registry (for parameter sources).
     * @return The current modulation value (typically 0-1 range).
     */
    [[nodiscard]] float getValue(const ParamRegistry* registry) const;
};

/**
 * @brief A modulation routing slot.
 */
struct ModSlot {
    ModSlotId id = InvalidModSlotId;
    ModSource source;
    ParamId target = InvalidParamId;
    float amount = 0.0f;
    BlendMode blendMode = BlendMode::Add;
    bool enabled = true;
};

/**
 * @brief Modulation matrix routing modulation sources to parameter targets.
 * 
 * Manages modulation connections and processes them each audio block.
 */
class ModMatrix {
public:
    /**
     * @brief Construct the modulation matrix.
     * @param registry The parameter registry for target parameters.
     */
    explicit ModMatrix(ParamRegistry* registry);
    ~ModMatrix();
    
    // Non-copyable, non-movable
    ModMatrix(const ModMatrix&) = delete;
    ModMatrix& operator=(const ModMatrix&) = delete;
    ModMatrix(ModMatrix&&) = delete;
    ModMatrix& operator=(ModMatrix&&) = delete;
    
    // =========================================================================
    // Connection Management
    // =========================================================================
    
    /**
     * @brief Connect a modulation source to a parameter target.
     * @param source The modulation source.
     * @param target The target parameter ID.
     * @param amount The modulation amount.
     * @param mode The blending mode.
     * @return The slot ID, or InvalidModSlotId on failure.
     */
    ModSlotId connect(const ModSource& source, ParamId target, 
                      float amount, BlendMode mode = BlendMode::Add);
    
    /**
     * @brief Disconnect a modulation slot.
     * @param slot The slot ID to disconnect.
     * @return true if the slot was disconnected.
     */
    bool disconnect(ModSlotId slot);
    
    /**
     * @brief Update the amount for a modulation slot.
     */
    void setAmount(ModSlotId slot, float amount);
    
    /**
     * @brief Update the blend mode for a modulation slot.
     */
    void setBlendMode(ModSlotId slot, BlendMode mode);
    
    /**
     * @brief Enable or disable a modulation slot.
     */
    void setEnabled(ModSlotId slot, bool enabled);
    
    /**
     * @brief Get a modulation slot by ID.
     * @return Pointer to the slot, or nullptr if not found.
     */
    [[nodiscard]] const ModSlot* getSlot(ModSlotId slot) const;
    
    /**
     * @brief Get all slots targeting a specific parameter.
     */
    [[nodiscard]] std::vector<const ModSlot*> getSlotsForTarget(ParamId target) const;
    
    /**
     * @brief Get the number of active modulation slots.
     */
    [[nodiscard]] std::size_t getSlotCount() const noexcept;
    
    // =========================================================================
    // Processing (call per audio block)
    // =========================================================================
    
    /**
     * @brief Process all modulations and update target parameters.
     * 
     * Should be called once per audio block before processing.
     * This is real-time safe (no allocations).
     */
    void process() noexcept;
    
    /**
     * @brief Clear all accumulated modulation amounts.
     * 
     * Called before process() to reset modulation state.
     */
    void clearModulations() noexcept;
    
private:
    /**
     * @brief Apply a single modulation slot.
     */
    void applyModulation(const ModSlot& slot, 
                         std::unordered_map<ParamId, float>& modAmounts) noexcept;
    
    ParamRegistry* registry_;
    std::unordered_map<ModSlotId, ModSlot> slots_;
    ModSlotId nextSlotId_ = 1;
};

} // namespace cppmusic::engine::parameters
