#pragma once

#include "../processors/AudioProcessorBase.h"

namespace daw::audio::synthesis
{

/**
 * @brief Synthesizer processor
 */
class Synthesizer : public processors::AudioProcessorBase
{
public:
    Synthesizer();
    ~Synthesizer() override = default;

    void prepareToPlay(double sampleRate, int maximumBlockSize) override;
    void processBlock(float* buffer, int numSamples) noexcept override;
    void reset() override;

private:
    // Synthesis implementation
};

} // namespace daw::audio::synthesis

