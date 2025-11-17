#pragma once

#include <atomic>
#include <cmath>
#include <algorithm>

namespace daw::audio::engine
{

/**
 * @brief Zipper-noise-free parameter smoother
 * 
 * Smooths parameter changes to prevent zipper noise.
 * Follows DAW_DEV_RULES: real-time safe, no allocations in processBlock.
 */
class ParameterSmoother
{
public:
    ParameterSmoother() noexcept
        : currentValue(0.0f)
        , targetValue(0.0f)
        , smoothingTime(0.01f)
        , sampleRate(44100.0)
        , smoothingCoeff(0.0f)
    {
    }

    ~ParameterSmoother() = default;

    // Non-copyable, movable
    ParameterSmoother(const ParameterSmoother&) = delete;
    ParameterSmoother& operator=(const ParameterSmoother&) = delete;
    ParameterSmoother(ParameterSmoother&&) noexcept = default;
    ParameterSmoother& operator=(ParameterSmoother&&) noexcept = default;

    /**
     * @brief Prepare smoother for playback
     * @param sampleRate Current sample rate
     * @param smoothingTime Smoothing time in seconds
     */
    void prepareToPlay(double sampleRate, float smoothingTime = 0.01f) noexcept
    {
        this->sampleRate = sampleRate;
        this->smoothingTime = smoothingTime;
        updateCoefficient();
    }

    /**
     * @brief Set target value (call from UI thread)
     */
    void setTargetValue(float target) noexcept
    {
        targetValue.store(target, std::memory_order_release);
    }

    /**
     * @brief Get current value
     */
    [[nodiscard]] float getCurrentValue() const noexcept
    {
        return currentValue;
    }

    /**
     * @brief Process smoothing for a block
     * @param output Output buffer (must be pre-allocated)
     * @param numSamples Number of samples
     */
    void processBlock(float* output, int numSamples) noexcept
    {
        const auto target = targetValue.load(std::memory_order_acquire);
        const auto coeff = smoothingCoeff;
        
        for (int i = 0; i < numSamples; ++i)
        {
            currentValue += (target - currentValue) * coeff;
            output[i] = currentValue;
        }
    }

    /**
     * @brief Reset to a specific value
     */
    void reset(float value = 0.0f) noexcept
    {
        currentValue = value;
        targetValue.store(value, std::memory_order_release);
    }

    /**
     * @brief Set smoothing time
     */
    void setSmoothingTime(float timeSeconds) noexcept
    {
        smoothingTime = timeSeconds;
        updateCoefficient();
    }

private:
    float currentValue;
    std::atomic<float> targetValue;
    float smoothingTime;
    double sampleRate;
    float smoothingCoeff;
    
    void updateCoefficient() noexcept
    {
        if (sampleRate > 0.0 && smoothingTime > 0.0f)
        {
            // Exponential smoothing coefficient
            const auto numSamples = smoothingTime * sampleRate;
            smoothingCoeff = numSamples > 0.0 ? static_cast<float>(1.0 / numSamples) : 1.0f;
            smoothingCoeff = std::min(smoothingCoeff, 1.0f);
        }
        else
        {
            smoothingCoeff = 1.0f; // No smoothing
        }
    }
};

} // namespace daw::audio::engine

