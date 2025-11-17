#include "Compressor.h"
#include <algorithm>
#include <cmath>

namespace daw::audio::effects
{

Compressor::Compressor()
{
    reset();
}

void Compressor::prepareToPlay(double sampleRate, int maximumBlockSize)
{
    AudioProcessorBase::prepareToPlay(sampleRate, maximumBlockSize);
    const float sr = static_cast<float>(std::max(sampleRate, 1.0));
    const float attackSamples = std::max(attackTime * sr, 1.0f);
    const float releaseSamples = std::max(releaseTime * sr, 1.0f);
    attackCoefficient = std::exp(-1.0f / attackSamples);
    releaseCoefficient = std::exp(-1.0f / releaseSamples);
    reset();
}

void Compressor::processBlock(float* buffer, int numSamples) noexcept
{
    // Real-time safe compression processing
    // No allocations, no locks, deterministic execution
    for (int i = 0; i < numSamples; ++i)
    {
        float input = buffer[i];
        float inputDb = 20.0f * std::log10(std::max(std::abs(input), 1e-10f));
        float gainLinear = 1.0f;
        
        if (inputDb > threshold)
        {
            float excess = inputDb - threshold;
            float compressedDb = threshold + (excess / ratio);
            float gainReduction = compressedDb - inputDb;
            gainLinear = std::pow(10.0f, gainReduction / 20.0f);
        }
        
        const float rawCoeff = (gainLinear < envelope ? attackCoefficient : releaseCoefficient);
        const float coeff = (rawCoeff > 0.0f && rawCoeff < 1.0f) ? rawCoeff : 0.99f;
        envelope = (coeff * envelope) + ((1.0f - coeff) * gainLinear);
        input *= envelope;

        // Prevent denormals
        buffer[i] = input + 1e-20f;
    }
}

void Compressor::reset()
{
    envelope = 1.0f;
}

void Compressor::setThreshold(float thresholdDb) noexcept
{
    threshold = std::clamp(thresholdDb, -60.0f, 0.0f);
}

void Compressor::setRatio(float newRatio) noexcept
{
    ratio = std::clamp(newRatio, 1.0f, 20.0f);
}

} // namespace daw::audio::effects

