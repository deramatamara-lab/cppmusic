#pragma once

#include <juce_core/juce_core.h>
#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <queue>
#include <chrono>

namespace daw::ai {

//==============================================================================
// Forward Declarations
//==============================================================================

class NeuralModel;
class InferenceWorker;
class ModelRegistry;

//==============================================================================
// Inference Metrics
//==============================================================================

struct InferenceMetrics
{
    std::atomic<float> inferenceTimeMs{0.0f};     // Average inference time in milliseconds
    std::atomic<float> throughput{0.0f};           // Inferences per second
    std::atomic<int> activeModels{0};             // Number of loaded models
    std::atomic<float> gpuMemoryUsageMb{0.0f};    // GPU memory usage in MB
    std::atomic<int> queueDepth{0};               // Number of queued inference requests
    std::atomic<int> activeWorkers{0};            // Number of active worker threads

    InferenceMetrics() = default;
};

//==============================================================================
// Inference Request/Response
//==============================================================================

struct InferenceRequest
{
    juce::String modelName;
    std::vector<float> input;
    std::function<void(const std::vector<float>&)> onComplete;
    std::function<void(const juce::String&)> onError;
    uint64_t requestId;
    std::chrono::high_resolution_clock::time_point timestamp;

    InferenceRequest() : requestId(0) {}
};

struct InferenceResponse
{
    uint64_t requestId;
    std::vector<float> output;
    float inferenceTimeMs;
    juce::String errorMessage;
    bool success;

    InferenceResponse() : requestId(0), inferenceTimeMs(0.0f), success(false) {}
};

//==============================================================================
// Neural Model Interface
//==============================================================================

class NeuralModel
{
public:
    enum class ModelType
    {
        MusicGeneration,
        AudioAnalysis,
        StyleTransfer,
        Harmonization,
        RhythmGeneration,
        MelodyGeneration,
        BassGeneration,
        DrumGeneration
    };

    virtual ~NeuralModel() = default;

    virtual bool load(const juce::String& modelPath) = 0;
    virtual void unload() = 0;
    virtual bool isLoaded() const = 0;

    virtual std::vector<float> infer(const std::vector<float>& input) = 0;
    virtual ModelType getType() const = 0;
    virtual juce::String getName() const = 0;

    virtual size_t getInputSize() const = 0;
    virtual size_t getOutputSize() const = 0;

    // GPU support (optional)
    virtual bool supportsGPU() const { return false; }
    virtual bool useGPU(bool enable) { return !enable; } // Return false if GPU not available

protected:
    NeuralModel() = default;
};

//==============================================================================
// Ultra Neural Inference Engine
//==============================================================================

class UltraNeuralInferenceEngine
{
public:
    using InferenceCallback = std::function<void(const std::vector<float>&)>;
    using ErrorCallback = std::function<void(const juce::String&)>;

    UltraNeuralInferenceEngine();
    ~UltraNeuralInferenceEngine();

    // Non-copyable, movable
    UltraNeuralInferenceEngine(const UltraNeuralInferenceEngine&) = delete;
    UltraNeuralInferenceEngine& operator=(const UltraNeuralInferenceEngine&) = delete;
    UltraNeuralInferenceEngine(UltraNeuralInferenceEngine&&) noexcept = default;
    UltraNeuralInferenceEngine& operator=(UltraNeuralInferenceEngine&&) noexcept = default;

    //==============================================================================
    // Lifecycle
    //==============================================================================

    bool initialize(int numWorkerThreads = 4);
    void shutdown();
    bool isInitialized() const noexcept { return initialized_.load(std::memory_order_acquire); }

    //==============================================================================
    // Model Management
    //==============================================================================

    bool loadModel(const juce::String& modelPath, const juce::String& modelName,
                  NeuralModel::ModelType type = NeuralModel::ModelType::MusicGeneration);

    void unloadModel(const juce::String& modelName);
    bool isModelLoaded(const juce::String& modelName) const;

    std::vector<juce::String> getLoadedModels() const;

    //==============================================================================
    // Inference Operations
    //==============================================================================

    // Synchronous inference
    std::vector<float> runInference(const juce::String& modelName, const std::vector<float>& input);

    // Asynchronous inference
    uint64_t runInferenceAsync(const juce::String& modelName, const std::vector<float>& input,
                              InferenceCallback onComplete, ErrorCallback onError = nullptr);

    // Cancel pending request
    void cancelInference(uint64_t requestId);

    //==============================================================================
    // Performance & Monitoring
    //==============================================================================

    const InferenceMetrics& getMetrics() const noexcept { return metrics_; }
    std::vector<InferenceResponse> getCompletedRequests();

    // GPU management
    bool enableGPU(bool enable);
    bool isGPUEnabled() const noexcept { return gpuEnabled_.load(std::memory_order_acquire); }
    float getGPUUtilization() const;

    //==============================================================================
    // Configuration
    //==============================================================================

    void setMaxQueueDepth(size_t depth);
    void setInferenceTimeout(std::chrono::milliseconds timeout);

private:
    //==============================================================================
    // Private Implementation
    //==============================================================================

    std::atomic<bool> initialized_{false};
    std::atomic<bool> gpuEnabled_{false};

    InferenceMetrics metrics_;

    // Model registry
    std::unique_ptr<ModelRegistry> modelRegistry_;

    // Worker threads
    std::vector<std::unique_ptr<InferenceWorker>> workers_;
    std::vector<std::thread> workerThreads_;

    // Request management
    std::queue<InferenceRequest> requestQueue_;
    std::unordered_map<uint64_t, InferenceResponse> completedRequests_;
    mutable std::mutex queueMutex_;
    mutable std::mutex completedMutex_;

    // Control
    std::atomic<bool> running_{false};
    std::condition_variable queueCondition_;
    std::mutex controlMutex_;

    // Configuration
    size_t maxQueueDepth_{1024};
    std::chrono::milliseconds inferenceTimeout_{5000};

    // Request ID generation
    std::atomic<uint64_t> nextRequestId_{1};

    //==============================================================================
    // Internal Methods
    //==============================================================================

    uint64_t generateRequestId();
    void workerThreadFunction(size_t workerId);
    void processInferenceRequest(const InferenceRequest& request);
    bool validateRequest(const InferenceRequest& request) const;
    void updateMetrics(const InferenceResponse& response);
    void cleanupCompletedRequests();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UltraNeuralInferenceEngine)
};

//==============================================================================
// Model Registry (Private Implementation)
//==============================================================================

class ModelRegistry
{
public:
    ModelRegistry();
    ~ModelRegistry();

    bool registerModel(const juce::String& name, std::unique_ptr<NeuralModel> model);
    void unregisterModel(const juce::String& name);
    NeuralModel* getModel(const juce::String& name) const;
    bool hasModel(const juce::String& name) const;

    std::vector<juce::String> getRegisteredModels() const;

private:
    std::unordered_map<juce::String, std::unique_ptr<NeuralModel>> models_;
    mutable std::mutex mutex_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelRegistry)
};

//==============================================================================
// Inference Worker (Private Implementation)
//==============================================================================

class InferenceWorker
{
public:
    InferenceWorker(UltraNeuralInferenceEngine* engine, size_t workerId);
    ~InferenceWorker();

    void start();
    void stop();
    void processRequest(const InferenceRequest& request);

private:
    UltraNeuralInferenceEngine* engine_;
    size_t workerId_;
    std::atomic<bool> running_{false};

    void workerLoop();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InferenceWorker)
};

//==============================================================================
// Concrete Model Implementations
//==============================================================================

class BasicNeuralModel : public NeuralModel
{
public:
    BasicNeuralModel(ModelType type, const juce::String& name);
    ~BasicNeuralModel() override;

    bool load(const juce::String& modelPath) override;
    void unload() override;
    bool isLoaded() const override { return loaded_; }

    std::vector<float> infer(const std::vector<float>& input) override;
    ModelType getType() const override { return type_; }
    juce::String getName() const override { return name_; }

    size_t getInputSize() const override { return inputSize_; }
    size_t getOutputSize() const override { return outputSize_; }

private:
    ModelType type_;
    juce::String name_;
    bool loaded_;
    size_t inputSize_;
    size_t outputSize_;

    // Mock model data (in real implementation, this would load actual model weights)
    std::vector<float> modelWeights_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BasicNeuralModel)
};

} // namespace daw::ai
