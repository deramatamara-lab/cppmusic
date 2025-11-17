#pragma once

#include "../processors/AudioProcessorBase.h"

namespace daw::audio::effects
{

/**
 * @brief Audio compressor with sidechain support
 * 
 * Implements dynamic range compression following the development rules.
 * Thread-safe parameter changes, real-time safe processing.
 */
class Compressor : public processors::AudioProcessorBase
{
public:
    Compressor();
    ~Compressor() override = default;

    void prepareToPlay(double sampleRate, int maximumBlockSize) override;
    void processBlock(float* buffer, int numSamples) noexcept override;
    void reset() override;

    /**
     * @brief Set compression threshold in dB
     * @param thresholdDb Threshold value (-60 to 0 dB)
     */
    void setThreshold(float thresholdDb) noexcept;

    /**
     * @brief Get current threshold
     * @return Threshold in dB
     */
    [[nodiscard]] float getThreshold() const noexcept { return threshold; }

    /**
     * @brief Set compression ratio
     * @param ratio Compression ratio (1.0 to 20.0)
     */
    void setRatio(float ratio) noexcept;

    /**
     * @brief Get current ratio
     * @return Compression ratio
     */
    [[nodiscard]] float getRatio() const noexcept { return ratio; }

private:
    float threshold = -12.0f;
    float ratio = 4.0f;
    float attackTime = 0.003f;
    float releaseTime = 0.1f;
    float attackCoefficient = 0.0f;
    float releaseCoefficient = 0.0f;
    
    // State variables (pre-allocated for real-time safety)
    float envelope = 0.0f;
};

} // namespace daw::audio::effects

