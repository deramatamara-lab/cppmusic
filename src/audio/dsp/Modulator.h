#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>

namespace daw::audio::dsp
{

/**
 * @brief Base modulator interface
 * 
 * All modulators derive from this base class.
 * Follows DAW_DEV_RULES: real-time safe, no allocations in processBlock.
 */
class Modulator
{
public:
    Modulator() noexcept = default;
    virtual ~Modulator() = default;

    // Non-copyable, non-movable (atomic members prevent safe moves)
    Modulator(const Modulator&) = delete;
    Modulator& operator=(const Modulator&) = delete;

    /**
     * @brief Prepare modulator for playback
     * @param sampleRate Current sample rate
     * @param maxBlockSize Maximum block size
     */
    virtual void prepareToPlay(double sampleRate, int maxBlockSize) noexcept = 0;

    /**
     * @brief Release resources
     */
    virtual void releaseResources() noexcept = 0;

    /**
     * @brief Reset modulator state
     */
    virtual void reset() noexcept = 0;

    /**
     * @brief Process modulation for a block
     * @param output Buffer to write modulation values (must be pre-allocated)
     * @param numSamples Number of samples to process
     * @return true if modulation is active, false otherwise
     */
    virtual bool processBlock(float* output, int numSamples) noexcept = 0;

    /**
     * @brief Get current modulation value (single sample)
     */
    [[nodiscard]] virtual float getCurrentValue() const noexcept = 0;

    /**
     * @brief Check if modulator is enabled
     */
    [[nodiscard]] bool isEnabled() const noexcept { return enabled.load(std::memory_order_acquire); }

    /**
     * @brief Set modulator enabled state
     */
    void setEnabled(bool newEnabled) noexcept { enabled.store(newEnabled, std::memory_order_release); }

    /**
     * @brief Get modulation depth (0.0 to 1.0)
     */
    [[nodiscard]] float getDepth() const noexcept { return depth.load(std::memory_order_acquire); }

    /**
     * @brief Set modulation depth (0.0 to 1.0)
     */
    void setDepth(float newDepth) noexcept { depth.store(std::clamp(newDepth, 0.0f, 1.0f), std::memory_order_release); }

protected:
    std::atomic<bool> enabled{true};
    std::atomic<float> depth{1.0f};
    double currentSampleRate{44100.0};
    int currentBlockSize{128};
};

} // namespace daw::audio::dsp

