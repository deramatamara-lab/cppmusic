#pragma once

namespace daw::audio::synthesis
{

/**
 * @brief Oscillator for waveform generation
 */
class Oscillator
{
public:
    Oscillator() = default;
    ~Oscillator() = default;

    void setFrequency(float frequency) noexcept;
    void setSampleRate(double sampleRate) noexcept;
    
    [[nodiscard]] float getNextSample() noexcept;

private:
    double sampleRate = 44100.0;
    float frequency = 440.0f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    
    void updatePhaseIncrement() noexcept;
};

} // namespace daw::audio::synthesis

