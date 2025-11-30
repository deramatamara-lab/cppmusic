#pragma once

#include "JuceHeader.h"
#include "../core/ServiceLocator.h"
#include "../core/EngineContext.h"
#include <atomic>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <future>

namespace daw::ai {

/**
 * Advanced Neural Inference Engine integrated with DAW architecture.
 *
 * Key features:
 * - Real-time inference with bounded latency guarantees
 * - GPU acceleration where available
 * - Multi-model parallel processing
 * - Streaming inference for continuous audio analysis
 * - Service-based architecture with dependency injection
 * - Comprehensive metrics and performance monitoring
 */
class NeuralInferenceService
{
public:
    /**
     * Inference request structure
     */
    struct InferenceRequest
    {
        uint32_t requestId;
        std::string modelName;
        std::vector<float> inputData;
        std::function<void(const std::vector<float>&)> onComplete;
        std::function<void(const std::string&)> onError;
        std::chrono::steady_clock::time_point submissionTime;

        // Performance hints
        enum Priority : uint8_t {
            Low = 0,        // Background processing
            Normal = 1,     // Standard inference
            High = 2,       // UI-blocking operations
            RealTime = 3    // Audio thread dependent (avoid if possible)
        };
        Priority priority = Normal;

        // Quality vs speed tradeoff
        enum Quality : uint8_t {
            Fast = 0,       // Reduced precision, smaller models
            Balanced = 1,   // Default quality/speed balance
            High = 2,       // Full precision, maximum quality
            Ultra = 3       // Maximum quality, no speed constraints
        };
        Quality quality = Balanced;
    };

    /**
     * Performance metrics updated in real-time
     */
    struct InferenceMetrics
    {
        std::atomic<float> averageLatencyMs{0.0f};
        std::atomic<float> p95LatencyMs{0.0f};
        std::atomic<float> throughputHz{0.0f};           // Inferences per second
        std::atomic<uint32_t> activeRequests{0};
        std::atomic<uint32_t> completedRequests{0};
        std::atomic<uint32_t> failedRequests{0};
        std::atomic<float> gpuMemoryUsageMB{0.0f};
        std::atomic<float> cpuUsagePercent{0.0f};
        std::atomic<bool> gpuAccelerated{false};

        // Queue health metrics
        std::atomic<uint32_t> queueDepth{0};
        std::atomic<uint32_t> queueOverflows{0};
        std::atomic<float> queueWaitTimeMs{0.0f};
    };

    /**
     * Model information and capabilities
     */
    struct ModelInfo
    {
        std::string name;
        std::string version;
        std::string architecture; // "transformer", "cnn", "rnn", etc.
        size_t inputDimensions;
        size_t outputDimensions;
        float expectedLatencyMs;
        bool supportsStreaming;
        bool requiresGPU;
        size_t memoryRequirementMB;

        enum Status {
            NotLoaded,
            Loading,
            Ready,
            Error
        };
        std::atomic<Status> status{NotLoaded};
    };

    NeuralInferenceService();
    ~NeuralInferenceService();

    //==============================================================================
    // Service Lifecycle
    //==============================================================================

    /**
     * Initialize the inference service
     * @param maxWorkerThreads Maximum number of worker threads (0 = auto-detect)
     * @param enableGPU Enable GPU acceleration if available
     * @return true if initialization succeeded
     */
    bool initialize(int maxWorkerThreads = 0, bool enableGPU = true);

    /**
     * Shutdown the service and cleanup resources
     */
    void shutdown();

    /**
     * Check if service is ready for inference
     */
    [[nodiscard]] bool isReady() const { return initialized.load(); }

    //==============================================================================
    // Model Management
    //==============================================================================

    /**
     * Load a neural network model
     * @param modelPath Path to model file
     * @param modelName Unique identifier for the model
     * @return true if model was loaded successfully
     */
    bool loadModel(const std::string& modelPath, const std::string& modelName);

    /**
     * Unload a model and free its resources
     * @param modelName Model to unload
     */
    void unloadModel(const std::string& modelName);

    /**
     * Get information about a loaded model
     * @param modelName Model to query
     * @return Model information, or nullptr if not found
     */
    [[nodiscard]] std::shared_ptr<ModelInfo> getModelInfo(const std::string& modelName) const;

    /**
     * Get list of all loaded models
     */
    [[nodiscard]] std::vector<std::string> getLoadedModels() const;

    //==============================================================================
    // Inference Operations
    //==============================================================================

    /**
     * Submit an inference request (asynchronous)
     * @param request Inference request with callbacks
     * @return Request ID for tracking, 0 if submission failed
     */
    [[nodiscard]] uint32_t submitInference(const InferenceRequest& request);

    /**
     * Synchronous inference (blocks until complete)
     * @param modelName Model to use
     * @param inputData Input tensor
     * @param quality Quality/speed tradeoff
     * @return Output tensor, empty on failure
     */
    [[nodiscard]] std::vector<float> runInference(
        const std::string& modelName,
        const std::vector<float>& inputData,
        InferenceRequest::Quality quality = InferenceRequest::Quality::Balanced);

    /**
     * Cancel a pending inference request
     * @param requestId Request to cancel
     * @return true if request was cancelled
     */
    bool cancelInference(uint32_t requestId);

    /**
     * Cancel all pending requests for a model
     * @param modelName Model to cancel requests for
     */
    void cancelModelInferences(const std::string& modelName);

    //==============================================================================
    // Performance Monitoring
    //==============================================================================

    /**
     * Get current performance metrics
     */
    const InferenceMetrics& getMetrics() const { return metrics; }

    /**
     * Set performance targets for adaptive quality scaling
     * @param targetLatencyMs Target P95 latency in milliseconds
     * @param maxQueueDepth Maximum allowed queue depth
     */
    void setPerformanceTargets(float targetLatencyMs, uint32_t maxQueueDepth);

    /**
     * Enable/disable adaptive quality scaling
     * @param enabled If true, automatically reduce quality when performance targets are missed
     */
    void setAdaptiveQuality(bool enabled) { adaptiveQuality.store(enabled); }

    //==============================================================================
    // Audio Integration
    //==============================================================================

    /**
     * Process real-time audio analysis requests
     * This method is optimized for low-latency audio thread communication
     * @param audioData Input audio samples
     * @param sampleRate Audio sample rate
     * @param onComplete Callback for results (called on background thread)
     */
    void processAudioStream(
        const std::vector<float>& audioData,
        double sampleRate,
        std::function<void(const core::Messages::AIResult&)> onComplete);

    /**
     * Set the engine context for audio thread communication
     */
    void setEngineContext(std::shared_ptr<core::EngineContext> context) { engineContext = context; }

private:
    std::atomic<bool> initialized{false};
    std::atomic<bool> adaptiveQuality{true};
    std::atomic<uint32_t> nextRequestId{1};

    // Performance targets
    std::atomic<float> targetLatencyMs{50.0f};
    std::atomic<uint32_t> maxQueueDepth{256};

    // Metrics
    InferenceMetrics metrics;

    // Model storage
    mutable std::mutex modelsMutex;
    std::unordered_map<std::string, std::shared_ptr<ModelInfo>> loadedModels;

    // Worker thread pool
    std::vector<std::unique_ptr<std::thread>> workerThreads;
    std::atomic<bool> shouldStop{false};

    // Request queue
    mutable std::mutex requestsMutex;
    std::queue<InferenceRequest> pendingRequests;
    std::condition_variable requestsCV;

    // Engine integration
    std::shared_ptr<core::EngineContext> engineContext;

    // GPU resources
    std::atomic<bool> gpuEnabled{false};
    std::atomic<bool> gpuAvailable{false};

    //==============================================================================
    // Internal Methods
    //==============================================================================

    // Worker thread main loop
    void workerThreadMain();

    // Process a single inference request
    void processInferenceRequest(const InferenceRequest& request);

    // GPU inference backend
    std::vector<float> runGPUInference(const std::string& modelName, const std::vector<float>& input);

    // CPU inference backend
    std::vector<float> runCPUInference(const std::string& modelName, const std::vector<float>& input);

    // Performance monitoring
    void updateMetrics(float latencyMs, bool success);
    void checkAndAdaptQuality();

    // Model loading implementation
    bool loadModelImpl(const std::string& modelPath, const std::string& modelName);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeuralInferenceService)
};

} // namespace daw::ai
