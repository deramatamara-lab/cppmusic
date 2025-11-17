#pragma once

#include <memory>

namespace daw::audio::processors
{

/**
 * @brief Base class for audio processors
 * 
 * This class provides the foundation for all audio processing components.
 * Follows the development rules for thread safety and memory management.
 */
class AudioProcessorBase
{
public:
    AudioProcessorBase() = default;
    virtual ~AudioProcessorBase() = default;

    // Non-copyable, movable
    AudioProcessorBase(const AudioProcessorBase&) = delete;
    AudioProcessorBase& operator=(const AudioProcessorBase&) = delete;
    AudioProcessorBase(AudioProcessorBase&&) noexcept = default;
    AudioProcessorBase& operator=(AudioProcessorBase&&) noexcept = default;

    /**
     * @brief Prepare the processor for playback
     * @param sampleRate Sample rate in Hz
     * @param maximumBlockSize Maximum buffer size in samples
     */
    virtual void prepareToPlay(double sampleRate, int maximumBlockSize) = 0;

    /**
     * @brief Process audio block (must be real-time safe)
     * @param buffer Audio buffer to process
     * @param numSamples Number of samples in buffer
     */
    virtual void processBlock(float* buffer, int numSamples) noexcept = 0;

    /**
     * @brief Reset processor state
     */
    virtual void reset() = 0;

protected:
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
};

} // namespace daw::audio::processors

