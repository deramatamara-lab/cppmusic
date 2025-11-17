#include "Delay.h"

namespace daw::audio::effects
{

Delay::Delay()
{
}

void Delay::prepareToPlay(double sampleRate, int maximumBlockSize)
{
    AudioProcessorBase::prepareToPlay(sampleRate, maximumBlockSize);
    reset();
}

void Delay::processBlock(float* /*buffer*/, int /*numSamples*/) noexcept
{
    // Delay processing implementation
    // Must be real-time safe
}

void Delay::reset()
{
    // Reset delay state
}

} // namespace daw::audio::effects

