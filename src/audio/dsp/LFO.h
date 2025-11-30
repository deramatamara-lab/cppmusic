#pragma once

#include "Modulator.h"
#include <atomic>
#include <cmath>

namespace daw::audio::dsp
{

/**
 * @brief Low-frequency oscillator modulator
 *
 * Supports multiple waveforms: sine, triangle, saw, square, noise, etc.
 * Follows DAW_DEV_RULES: real-time safe, no allocations in processBlock.
 */
class LFO : public Modulator
{
public:
    enum class Waveform
    {
        Sine,
        Triangle,
        Sawtooth,
        SawtoothInverse,
        Square,
        Pulse,
        Noise,
        SampleAndHold
    };

    LFO() noexcept;
    ~LFO() override = default;

    void prepareToPlay(double sampleRate, int maxBlockSize) noexcept override;
    void releaseResources() noexcept override;
    void reset() noexcept override;
    bool processBlock(float* output, int numSamples) noexcept override;
    [[nodiscard]] float getCurrentValue() const noexcept override;

    /**
     * @brief Set LFO frequency in Hz
     */
    void setFrequency(float frequencyHz) noexcept;

    /**
     * @brief Get LFO frequency in Hz
     */
    [[nodiscard]] float getFrequency() const noexcept;

    /**
     * @brief Set waveform type
     */
    void setWaveform(Waveform waveform) noexcept;

    /**
     * @brief Get waveform type
     */
    [[nodiscard]] Waveform getWaveform() const noexcept;

    /**
     * @brief Set phase offset (0.0 to 1.0)
     */
    void setPhaseOffset(float phaseOffset) noexcept;

    /**
     * @brief Get phase offset
     */
    [[nodiscard]] float getPhaseOffset() const noexcept;

    /**
     * @brief Set pulse width (for pulse waveform, 0.0 to 1.0)
     */
    void setPulseWidth(float pulseWidth) noexcept;

    /**
     * @brief Get pulse width
     */
    [[nodiscard]] float getPulseWidth() const noexcept;

    /**
     * @brief Sync to tempo (BPM)
     */
    void setSyncToTempo(bool sync, float tempoBpm, float beatDivision = 1.0f) noexcept;

private:
    std::atomic<float> frequencyHz{1.0f};
    std::atomic<Waveform> waveform{Waveform::Sine};
    std::atomic<float> phaseOffset{0.0f};
    std::atomic<float> pulseWidth{0.5f};
    std::atomic<bool> syncToTempo{false};
    std::atomic<float> syncTempoBpm{120.0f};
    std::atomic<float> syncBeatDivision{1.0f};

    float phase{0.0f};
    mutable float lastNoiseValue{0.0f};
    mutable float lastSampleHoldValue{0.0f};
    mutable int sampleHoldCounter{0};

    [[nodiscard]] float generateSample(float phaseValue, Waveform wf) const noexcept;
    [[nodiscard]] float getPhaseIncrement() const noexcept;
};

} // namespace daw::audio::dsp

