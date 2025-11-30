#pragma once

#include <cstdint>

namespace daw::audio::synthesis
{

/**
 * @brief Oscillator for waveform generation
 *
 * Supports multiple waveform types: sine, square, sawtooth, triangle, noise.
 * Real-time safe, no allocations in getNextSample().
 */
class Oscillator
{
public:
    enum class Waveform
    {
        Sine,
        Square,
        Sawtooth,
        Triangle,
        Noise
    };

    Oscillator() = default;
    ~Oscillator() = default;

    void setFrequency(float frequency) noexcept;
    void setSampleRate(double sampleRate) noexcept;
    void setWaveform(Waveform waveform) noexcept;

    [[nodiscard]] float getNextSample() noexcept;
    [[nodiscard]] Waveform getWaveform() const noexcept { return currentWaveform; }

private:
    double sampleRate = 44100.0;
    float frequency = 440.0f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    Waveform currentWaveform = Waveform::Sine;

    // Noise generation state (real-time safe LCG)
    uint32_t noiseSeed = 12345;

    void updatePhaseIncrement() noexcept;
    [[nodiscard]] float generateSine() noexcept;
    [[nodiscard]] float generateSquare() noexcept;
    [[nodiscard]] float generateSawtooth() noexcept;
    [[nodiscard]] float generateTriangle() noexcept;
    [[nodiscard]] float generateNoise() noexcept;
};

} // namespace daw::audio::synthesis
