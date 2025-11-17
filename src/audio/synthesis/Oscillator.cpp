#include "Oscillator.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace daw::audio::synthesis
{

void Oscillator::setFrequency(float newFrequency) noexcept
{
    frequency = newFrequency;
    updatePhaseIncrement();
}

void Oscillator::setSampleRate(double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    updatePhaseIncrement();
}

float Oscillator::getNextSample() noexcept
{
    float sample = std::sin(phase);
    phase += phaseIncrement;
    
    // Wrap phase
    constexpr float twoPi = 2.0f * static_cast<float>(M_PI);
    if (phase >= twoPi)
        phase -= twoPi;
    
    return sample;
}

void Oscillator::updatePhaseIncrement() noexcept
{
    constexpr float twoPi = 2.0f * static_cast<float>(M_PI);
    phaseIncrement = twoPi * frequency / static_cast<float>(sampleRate);
}

} // namespace daw::audio::synthesis

