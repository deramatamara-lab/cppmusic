#include "Oscillator.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace daw::audio::synthesis
{

void Oscillator::setFrequency(float newFrequency) noexcept
{
    frequency = std::max(0.0f, std::min(20000.0f, newFrequency));
    updatePhaseIncrement();
}

void Oscillator::setSampleRate(double newSampleRate) noexcept
{
    sampleRate = std::max(1.0, newSampleRate);
    updatePhaseIncrement();
}

void Oscillator::setWaveform(Waveform waveform) noexcept
{
    currentWaveform = waveform;
}

float Oscillator::getNextSample() noexcept
{
    float sample = 0.0f;
    
    switch (currentWaveform)
    {
        case Waveform::Sine:
            sample = generateSine();
            break;
        case Waveform::Square:
            sample = generateSquare();
            break;
        case Waveform::Sawtooth:
            sample = generateSawtooth();
            break;
        case Waveform::Triangle:
            sample = generateTriangle();
            break;
        case Waveform::Noise:
            sample = generateNoise();
            break;
    }
    
    return sample;
}

void Oscillator::updatePhaseIncrement() noexcept
{
    constexpr float twoPi = 2.0f * static_cast<float>(M_PI);
    phaseIncrement = twoPi * frequency / static_cast<float>(sampleRate);
}

float Oscillator::generateSine() noexcept
{
    const float sample = std::sin(phase);
    phase += phaseIncrement;
    
    // Wrap phase
    constexpr float twoPi = 2.0f * static_cast<float>(M_PI);
    if (phase >= twoPi)
        phase -= twoPi;
    
    return sample;
}

float Oscillator::generateSquare() noexcept
{
    constexpr float twoPi = 2.0f * static_cast<float>(M_PI);
    const float normalizedPhase = phase / twoPi;
    const float sample = (normalizedPhase < 0.5f) ? 1.0f : -1.0f;
    
    phase += phaseIncrement;
    if (phase >= twoPi)
        phase -= twoPi;
    
    return sample;
}

float Oscillator::generateSawtooth() noexcept
{
    constexpr float twoPi = 2.0f * static_cast<float>(M_PI);
    const float normalizedPhase = phase / twoPi;
    const float sample = 2.0f * normalizedPhase - 1.0f;
    
    phase += phaseIncrement;
    if (phase >= twoPi)
        phase -= twoPi;
    
    return sample;
}

float Oscillator::generateTriangle() noexcept
{
    constexpr float twoPi = 2.0f * static_cast<float>(M_PI);
    const float normalizedPhase = phase / twoPi;
    const float sample = (normalizedPhase < 0.5f) 
        ? (4.0f * normalizedPhase - 1.0f)
        : (3.0f - 4.0f * normalizedPhase);
    
    phase += phaseIncrement;
    if (phase >= twoPi)
        phase -= twoPi;
    
    return sample;
}

float Oscillator::generateNoise() noexcept
{
    // Linear congruential generator (real-time safe, deterministic)
    noiseSeed = noiseSeed * 1103515245u + 12345u;
    const auto normalized = static_cast<float>(noiseSeed & 0x7FFFFFFFu) / 2147483647.0f;
    return normalized * 2.0f - 1.0f;
}

} // namespace daw::audio::synthesis

