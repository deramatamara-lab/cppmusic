#pragma once

#include "../dsp/Modulator.h"
#include <vector>
#include <memory>
#include <atomic>
#include <cstdint>

namespace daw::audio::engine
{

/**
 * @brief Modulation matrix for routing modulators to parameters
 * 
 * Supports 64+ modulation slots per track.
 * Follows DAW_DEV_RULES: real-time safe, lock-free parameter updates.
 */
class ModulationMatrix
{
public:
    struct ModulationSlot
    {
        daw::audio::dsp::Modulator* modulator{nullptr};
        float* targetParameter{nullptr};
        float depth{1.0f};
        bool enabled{true};
    };

    ModulationMatrix() noexcept;
    ~ModulationMatrix() = default;

    // Non-copyable, movable
    ModulationMatrix(const ModulationMatrix&) = delete;
    ModulationMatrix& operator=(const ModulationMatrix&) = delete;
    ModulationMatrix(ModulationMatrix&&) noexcept = default;
    ModulationMatrix& operator=(ModulationMatrix&&) noexcept = default;

    /**
     * @brief Prepare modulation matrix for playback
     * @param sampleRate Current sample rate
     * @param maxBlockSize Maximum block size
     */
    void prepareToPlay(double sampleRate, int maxBlockSize) noexcept;

    /**
     * @brief Release resources
     */
    void releaseResources() noexcept;

    /**
     * @brief Add modulation slot
     * @param modulator Modulator to use
     * @param targetParameter Pointer to parameter to modulate
     * @param depth Modulation depth (0.0 to 1.0)
     * @return Slot index, or -1 if failed
     */
    int addSlot(daw::audio::dsp::Modulator* modulator, float* targetParameter, float depth = 1.0f) noexcept;

    /**
     * @brief Remove modulation slot
     * @param slotIndex Slot index to remove
     */
    void removeSlot(int slotIndex) noexcept;

    /**
     * @brief Enable/disable modulation slot
     * @param slotIndex Slot index
     * @param enabled Enabled state
     */
    void setSlotEnabled(int slotIndex, bool enabled) noexcept;

    /**
     * @brief Set modulation depth for slot
     * @param slotIndex Slot index
     * @param depth Modulation depth (0.0 to 1.0)
     */
    void setSlotDepth(int slotIndex, float depth) noexcept;

    /**
     * @brief Process modulation for a block
     * @param numSamples Number of samples to process
     * @param baseValues Base parameter values (input/output)
     */
    void processBlock(int numSamples, float* baseValues) noexcept;

    /**
     * @brief Get number of active slots
     */
    [[nodiscard]] int getNumSlots() const noexcept { return static_cast<int>(slots.size()); }

    /**
     * @brief Clear all slots
     */
    void clear() noexcept;

private:
    static constexpr int maxSlots = 64;
    
    std::vector<ModulationSlot> slots;
    std::vector<float> modulationBuffer;
    
    double currentSampleRate{44100.0};
    int currentBlockSize{128};
    
    void processSlot(const ModulationSlot& slot, int numSamples, float* baseValues) noexcept;
};

} // namespace daw::audio::engine

