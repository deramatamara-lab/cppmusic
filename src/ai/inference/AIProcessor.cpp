#include "AIProcessor.h"

namespace daw::ai::inference
{

AIProcessor::AIProcessor()
    : inferenceEngine(std::make_unique<InferenceEngine>(1))
{
}

AIProcessor::~AIProcessor()
{
    inferenceEngine->stop();
}

void AIProcessor::processAudio(float* /*buffer*/, int /*numSamples*/) noexcept
{
    // Audio thread - only read results
    if (resultsReady.load(std::memory_order_acquire))
    {
        // Apply AI results to buffer
        // Implementation would apply results here
        resultsReady.store(false, std::memory_order_release);
    }
}

void AIProcessor::requestAIProcessing(const std::vector<float>& input)
{
    // UI thread - queue request
    InferenceRequest request;
    request.inputData = input;
    request.callback = [this](std::vector<float> results)
    {
        std::lock_guard<std::mutex> lock(resultsLock);
        aiResults = std::move(results);
        resultsReady.store(true, std::memory_order_release);
    };
    
    inferenceEngine->queueInference(std::move(request));
}

} // namespace daw::ai::inference
