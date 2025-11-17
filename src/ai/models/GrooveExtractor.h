#pragma once

#include "../inference/InferenceEngine.h"
#include <vector>
#include <memory>
#include <functional>

namespace daw::ai::models
{

/**
 * @brief Groove extraction from audio
 * 
 * Extracts swing/groove quantization from audio with style matching.
 * Follows DAW_DEV_RULES: runs on background thread, non-blocking.
 */
class GrooveExtractor
{
public:
    struct GrooveResult
    {
        std::vector<double> swingValues; // Per-step swing amounts
        double overallSwing{0.0};
        std::string style{"straight"};
        bool success{false};
    };

    explicit GrooveExtractor(std::shared_ptr<daw::ai::inference::InferenceEngine> engine);
    ~GrooveExtractor() = default;

    /**
     * @brief Extract groove from audio buffer
     * @param audioData Audio samples (mono or stereo)
     * @param sampleRate Sample rate
     * @param callback Callback with result
     */
    void extractGroove(const std::vector<float>& audioData, double sampleRate,
                       std::function<void(GrooveResult)> callback);

private:
    std::shared_ptr<daw::ai::inference::InferenceEngine> inferenceEngine;
};

} // namespace daw::ai::models

