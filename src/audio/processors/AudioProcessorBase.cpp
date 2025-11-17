#include "AudioProcessorBase.h"

namespace daw::audio::processors
{

void AudioProcessorBase::prepareToPlay(double sampleRate, int maximumBlockSize)
{
    currentSampleRate = sampleRate;
    currentBlockSize = maximumBlockSize;
}

void AudioProcessorBase::reset()
{
    // Reset implementation
}

} // namespace daw::audio::processors

