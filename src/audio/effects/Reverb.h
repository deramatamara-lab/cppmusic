#pragma once

#include "../processors/AudioProcessorBase.h"

namespace daw::audio::effects
{

/**
 * @brief Reverb effect processor
 */
class Reverb : public processors::AudioProcessorBase
{
public:
    Reverb();
    ~Reverb() override = default;

    void prepareToPlay(double sampleRate, int maximumBlockSize) override;
    void processBlock(float* buffer, int numSamples) noexcept override;
    void reset() override;

private:
    // Reverb implementation
};

} // namespace daw::audio::effects

