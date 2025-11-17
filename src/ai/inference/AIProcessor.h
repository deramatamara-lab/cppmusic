#pragma once

#include "../models/AIModel.h"
#include "InferenceEngine.h"
#include <atomic>
#include <memory>

namespace daw::ai::inference
{

/**
 * @brief AI processor with async inference
 * 
 * NEVER runs AI inference on audio thread.
 * Uses message passing and atomic flags for thread-safe communication.
 */
class AIProcessor
{
public:
    AIProcessor();
    ~AIProcessor();

    /**
     * @brief Process audio (audio thread - only reads results)
     * @param buffer Audio buffer
     * @param numSamples Number of samples
     */
    void processAudio(float* buffer, int numSamples) noexcept;

    /**
     * @brief Request AI processing (UI thread - queues request)
     * @param input Input audio data
     */
    void requestAIProcessing(const std::vector<float>& input);

private:
    std::unique_ptr<InferenceEngine> inferenceEngine;
    std::shared_ptr<models::AIModel> currentModel;
    
    std::atomic<bool> resultsReady{false};
    std::vector<float> aiResults;
    std::mutex resultsLock;
};

} // namespace daw::ai::inference

