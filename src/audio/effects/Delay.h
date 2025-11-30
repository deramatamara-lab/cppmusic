#pragma once

#include "../processors/AudioProcessorBase.h"
#include <atomic>
#include <vector>
#include <cmath>

namespace daw::audio::effects
{

/**
 * @brief Professional delay effect processor
 *
 * Multi-tap delay with feedback, mix control, and high-quality interpolation.
 * Real-time safe, lock-free parameter updates, pre-allocated buffers.
 *
 * Features:
 * - Configurable delay time (0-2000ms)
 * - Feedback control (0-100%)
 * - Dry/wet mix control
 * - Linear interpolation for smooth delay changes
 * - Maximum delay: 2 seconds at 192kHz
 *
 * Follows DAW_DEV_RULES: no allocations in processBlock, thread-safe parameters.
 */
class Delay : public processors::AudioProcessorBase
{
public:
    Delay();
    ~Delay() override = default;

    void prepareToPlay(double sampleRate, int maximumBlockSize) override;
    void processBlock(float* buffer, int numSamples) noexcept override;
    void reset() override;

    /**
     * @brief Set delay time in milliseconds
     * @param delayMs Delay time (0.0 to 2000.0 ms)
     */
    void setDelayTime(float delayMs) noexcept;

    /**
     * @brief Get current delay time
     * @return Delay time in milliseconds
     */
    [[nodiscard]] float getDelayTime() const noexcept;

    /**
     * @brief Set feedback amount (0.0 to 1.0)
     * @param feedback Feedback amount (0.0 = no feedback, 1.0 = 100% feedback)
     */
    void setFeedback(float feedback) noexcept;

    /**
     * @brief Get current feedback amount
     * @return Feedback amount (0.0 to 1.0)
     */
    [[nodiscard]] float getFeedback() const noexcept;

    /**
     * @brief Set dry/wet mix (0.0 = dry, 1.0 = wet)
     * @param mix Mix amount (0.0 to 1.0)
     */
    void setMix(float mix) noexcept;

    /**
     * @brief Get current mix
     * @return Mix amount (0.0 to 1.0)
     */
    [[nodiscard]] float getMix() const noexcept;

private:
    static constexpr float MAX_DELAY_MS = 2000.0f;
    static constexpr float MIN_DELAY_MS = 0.0f;
    static constexpr float DENORMAL_PREVENTION = 1e-20f;

    // Thread-safe parameters (updated from UI thread)
    std::atomic<float> delayTimeMs{100.0f};
    std::atomic<float> feedbackAmount{0.3f};
    std::atomic<float> mixAmount{0.5f};

    // Delay buffer (pre-allocated for real-time safety)
    std::vector<float> delayBuffer;
    int writePosition{0};
    int bufferSize{0};

    // Current delay in samples (smoothly interpolated)
    float currentDelaySamples{0.0f};
    float targetDelaySamples{0.0f};

    // Smooth interpolation rate (samples per sample for delay time changes)
    static constexpr float INTERPOLATION_RATE = 0.001f;

    void updateDelaySamples() noexcept;
    [[nodiscard]] float readDelay(float delaySamples) const noexcept;
};

} // namespace daw::audio::effects

