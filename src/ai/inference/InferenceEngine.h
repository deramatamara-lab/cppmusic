#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <memory>

#include "../config/AIConfig.h"

namespace daw::ai::config
{
    class AIBackend;
    class AIConfig;
}

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
     * @brief Initialize with AI config
     */
    void initialize(std::shared_ptr<daw::ai::config::AIConfig> config);

    /**
     * @brief Queue inference request
     * @param request Inference request with input data and callback
     */
    void queueInference(InferenceRequest request);

    /**
     * @brief Queue text-based inference (for LLM backends)
     * @param prompt Text prompt
     * @param callback Result callback
     */
    void queueTextInference(const std::string& prompt,
                           std::function<void(const std::string&, bool)> callback);

    /**
     * @brief Stop inference engine and wait for threads
     */
    void stop();

    /**
     * @brief Check if engine is ready
     */
    [[nodiscard]] bool isReady() const { return backend != nullptr && backend->isAvailable(); }

private:
    void inferenceWorker();

    std::vector<float> performInference(const InferenceRequest& request);

    std::vector<std::thread> inferenceThreads;
    std::queue<InferenceRequest> inferenceQueue;
    std::mutex queueLock;
    std::condition_variable conditionVariable;
    std::atomic<bool> shouldStop{false};
    int numThreads{1};
    static constexpr size_t maxQueueSize = 4; // Bounded queue per DAW_DEV_RULES

    std::shared_ptr<daw::ai::config::AIBackend> backend;
    std::shared_ptr<daw::ai::config::AIConfig> config;
};

} // namespace daw::ai::inference

