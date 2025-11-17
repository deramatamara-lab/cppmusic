#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace daw::ai::inference
{

struct InferenceRequest
{
    std::vector<float> inputData;
    std::function<void(std::vector<float>)> callback;
};

/**
 * @brief AI inference engine
 * 
 * Runs inference on dedicated thread(s), separate from audio and UI threads.
 * Thread-safe queue for inference requests.
 */
class InferenceEngine
{
public:
    explicit InferenceEngine(int numThreads = 1);
    ~InferenceEngine();

    /**
     * @brief Queue inference request
     * @param request Inference request with input data and callback
     */
    void queueInference(InferenceRequest request);

    /**
     * @brief Stop inference engine and wait for threads
     */
    void stop();

private:
    void inferenceWorker();

    std::vector<float> performInference(const InferenceRequest& request);

    std::vector<std::thread> inferenceThreads;
    std::queue<InferenceRequest> inferenceQueue;
    std::mutex queueLock;
    std::condition_variable conditionVariable;
    std::atomic<bool> shouldStop{false};
};

} // namespace daw::ai::inference

