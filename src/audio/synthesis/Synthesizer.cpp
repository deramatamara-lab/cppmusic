#include "Synthesizer.h"

namespace daw::audio::synthesis
{

Synthesizer::Synthesizer()
{
}

void Synthesizer::prepareToPlay(double sampleRate, int maximumBlockSize)
{
    AudioProcessorBase::prepareToPlay(sampleRate, maximumBlockSize);
    reset();
}

void Synthesizer::processBlock(float* /*buffer*/, int /*numSamples*/) noexcept
{
    // Synthesis processing implementation
    // Must be real-time safe
}

void Synthesizer::reset()
{
    // Reset synthesis state
}

} // namespace daw::audio::synthesis

