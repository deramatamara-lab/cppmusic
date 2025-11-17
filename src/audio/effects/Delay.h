#pragma once

#include "../processors/AudioProcessorBase.h"

namespace daw::audio::effects
{

/**
 * @brief Delay effect processor
 */
class Delay : public processors::AudioProcessorBase
{
public:
    Delay();
    ~Delay() override = default;

    void prepareToPlay(double sampleRate, int maximumBlockSize) override;
    void processBlock(float* buffer, int numSamples) noexcept override;
    void reset() override;

private:
    // Delay implementation
};

} // namespace daw::audio::effects

