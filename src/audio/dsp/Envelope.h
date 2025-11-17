#pragma once

#include "Modulator.h"
#include <atomic>
#include <cmath>

namespace daw::audio::dsp
{

/**
 * @brief ADSR envelope modulator
 * 
 * Supports ADSR and multi-stage envelopes with curve shaping.
 * Follows DAW_DEV_RULES: real-time safe, no allocations in processBlock.
 */
class Envelope : public Modulator
{
public:
    enum class Stage
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    Envelope() noexcept;
    ~Envelope() override = default;

    void prepareToPlay(double sampleRate, int maxBlockSize) noexcept override;
    void releaseResources() noexcept override;
    void reset() noexcept override;
    bool processBlock(float* output, int numSamples) noexcept override;
    [[nodiscard]] float getCurrentValue() const noexcept override;

    /**
     * @brief Trigger envelope (start attack)
     */
    void trigger() noexcept;

    /**
     * @brief Release envelope (start release)
     */
    void release() noexcept;

    /**
     * @brief Check if envelope is active
     */
    [[nodiscard]] bool isActive() const noexcept;

    /**
     * @brief Set attack time in seconds
     */
    void setAttackTime(float attackSeconds) noexcept;

    /**
     * @brief Set decay time in seconds
     */
    void setDecayTime(float decaySeconds) noexcept;

    /**
     * @brief Set sustain level (0.0 to 1.0)
     */
    void setSustainLevel(float sustainLevel) noexcept;

    /**
     * @brief Set release time in seconds
     */
    void setReleaseTime(float releaseSeconds) noexcept;

    /**
     * @brief Set attack curve (0.0 = linear, >0 = exponential)
     */
    void setAttackCurve(float curve) noexcept;

    /**
     * @brief Set decay/release curve (0.0 = linear, >0 = exponential)
     */
    void setDecayCurve(float curve) noexcept;

    /**
     * @brief Get current stage
     */
    [[nodiscard]] Stage getCurrentStage() const noexcept { return currentStage; }

private:
    std::atomic<float> attackTime{0.01f};
    std::atomic<float> decayTime{0.1f};
    std::atomic<float> sustainLevel{0.7f};
    std::atomic<float> releaseTime{0.2f};
    std::atomic<float> attackCurve{1.0f};
    std::atomic<float> decayCurve{1.0f};
    
    Stage currentStage{Stage::Idle};
    float currentValue{0.0f};
    float stageProgress{0.0f};
    bool triggered{false};
    bool released{false};
    
    float attackIncrement{0.0f};
    float decayIncrement{0.0f};
    float releaseIncrement{0.0f};
    
    void updateIncrements() noexcept;
    [[nodiscard]] float applyCurve(float value, float curve) const noexcept;
};

} // namespace daw::audio::dsp

