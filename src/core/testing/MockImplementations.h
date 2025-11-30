#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <atomic>

namespace daw::core::testing {

/**
 * @brief Mock audio buffer generator for testing
 *
 * Generates predictable audio test signals without JUCE dependencies:
 * - Sine waves at specified frequencies
 * - White/pink noise
 * - Impulse responses
 * - Silence
 */
class MockAudioBufferGenerator
{
public:
    enum SignalType
    {
        Silence,
        SineWave,
        SquareWave,
        SawtoothWave,
        TriangleWave,
        WhiteNoise,
        PinkNoise,
        Impulse
    };

    struct Config
    {
        SignalType type = SineWave;
        double frequency = 440.0;     // Hz
        double amplitude = 1.0;       // 0.0 to 1.0
        double sampleRate = 44100.0;
        int numChannels = 2;
        int numSamples = 1024;
        uint32_t seed = 12345;        // For noise generation
    };

    MockAudioBufferGenerator(const Config& config = {});
    ~MockAudioBufferGenerator() = default;

    /**
     * @brief Generate audio buffer
     * @param buffer Output buffer (will be resized if necessary)
     */
    void generate(std::vector<float>& buffer);

    /**
     * @brief Generate multi-channel audio
     * @param buffers Vector of channel buffers
     */
    void generate(std::vector<std::vector<float>>& buffers);

    /**
     * @brief Update configuration
     */
    void setConfig(const Config& config);

    /**
     * @brief Get current configuration
     */
    const Config& getConfig() const noexcept { return config_; }

private:
    Config config_;
    double phase_;              // For oscillator signals
    uint32_t noiseState_;       // For noise generation

    void generateSilence(std::vector<float>& buffer);
    void generateSineWave(std::vector<float>& buffer);
    void generateSquareWave(std::vector<float>& buffer);
    void generateSawtoothWave(std::vector<float>& buffer);
    void generateTriangleWave(std::vector<float>& buffer);
    void generateWhiteNoise(std::vector<float>& buffer);
    void generatePinkNoise(std::vector<float>& buffer);
    void generateImpulse(std::vector<float>& buffer);

    // Noise generation helpers
    uint32_t lcgRandom();
    float generatePinkSample();
};

/**
 * @brief Mock inference client for AI testing
 *
 * Simulates AI inference responses without actual model execution:
 * - Configurable response delays
 * - Deterministic or random responses
 * - Error simulation
 * - Performance metrics collection
 */
class MockInferenceClient
{
public:
    struct Config
    {
        double baseLatencyMs = 10.0;      // Base response time
        double latencyVarianceMs = 5.0;   // Random latency variation
        bool deterministic = true;        // Use fixed responses
        uint32_t seed = 54321;           // For random responses
        bool simulateErrors = false;     // Occasionally return errors
        float errorRate = 0.05f;         // Error probability (0.0-1.0)
    };

    struct InferenceRequest
    {
        std::string prompt;
        std::vector<float> parameters;
        uint64_t timestamp;
    };

    struct InferenceResponse
    {
        bool success;
        std::string result;
        std::vector<float> embeddings;
        float confidence;
        uint64_t processingTimeMs;
        std::string errorMessage;
    };

    using ResponseCallback = std::function<void(const InferenceResponse&)>;

    MockInferenceClient(const Config& config = {});
    ~MockInferenceClient() = default;

    /**
     * @brief Submit inference request
     * @param request The inference request
     * @param callback Called when response is ready
     * @return true if request was accepted
     */
    bool submitRequest(const InferenceRequest& request, ResponseCallback callback);

    /**
     * @brief Cancel pending requests
     */
    void cancelAllRequests();

    /**
     * @brief Check if client is ready for requests
     */
    bool isReady() const noexcept;

    /**
     * @brief Get performance statistics
     */
    struct Stats
    {
        size_t totalRequests;
        size_t successfulRequests;
        size_t failedRequests;
        double averageLatencyMs;
        double minLatencyMs;
        double maxLatencyMs;
        double p95LatencyMs;
    };

    Stats getStats() const noexcept;

    /**
     * @brief Reset statistics
     */
    void resetStats();

private:
    Config config_;
    std::atomic<size_t> activeRequests_;
    Stats stats_;
    std::vector<std::pair<InferenceRequest, ResponseCallback>> pendingRequests_;
    std::mutex mutex_;

    void processRequest(const InferenceRequest& request, ResponseCallback callback);
    InferenceResponse generateMockResponse(const InferenceRequest& request);
    void updateStats(const InferenceResponse& response);
};

/**
 * @brief Mock device manager for audio testing
 *
 * Simulates audio device behavior without actual hardware:
 * - Configurable device properties
 * - Simulated audio callbacks
 * - Buffer size and latency control
 */
class MockDeviceManager
{
public:
    struct DeviceInfo
    {
        std::string name;
        int inputChannels;
        int outputChannels;
        std::vector<double> supportedSampleRates;
        std::vector<int> supportedBufferSizes;
        double defaultSampleRate;
        int defaultBufferSize;
    };

    struct Config
    {
        DeviceInfo deviceInfo;
        bool simulateDropouts = false;
        float dropoutRate = 0.001f;     // Buffer underrun probability
        uint32_t seed = 98765;
    };

    MockDeviceManager(const Config& config = {});
    ~MockDeviceManager() = default;

    /**
     * @brief Initialize mock device
     * @param numInputChannels Number of input channels to request
     * @param numOutputChannels Number of output channels to request
     * @return true if successful
     */
    bool initialize(int numInputChannels, int numOutputChannels);

    /**
     * @brief Start audio processing
     */
    bool start();

    /**
     * @brief Stop audio processing
     */
    void stop();

    /**
     * @brief Set audio callback
     */
    void setAudioCallback(std::function<void(const float**, int, float**, int, int)> callback);

    /**
     * @brief Get current device info
     */
    const DeviceInfo& getDeviceInfo() const noexcept { return config_.deviceInfo; }

    /**
     * @brief Get current sample rate
     */
    double getSampleRate() const noexcept { return config_.deviceInfo.defaultSampleRate; }

    /**
     * @brief Get current buffer size
     */
    int getBufferSize() const noexcept { return config_.deviceInfo.defaultBufferSize; }

    /**
     * @brief Check if device is active
     */
    bool isActive() const noexcept { return isActive_; }

private:
    Config config_;
    std::atomic<bool> isActive_;
    std::function<void(const float**, int, float**, int, int)> audioCallback_;
    std::vector<float> inputBuffer_;
    std::vector<float> outputBuffer_;
    uint32_t randomState_;

    void generateTestAudio();
    uint32_t lcgRandom();
};

/**
 * @brief Mock performance monitor for testing
 *
 * Captures and analyzes performance metrics without system dependencies.
 */
class MockPerformanceMonitor
{
public:
    struct Metrics
    {
        double cpuLoad;
        uint64_t xrunCount;
        std::chrono::nanoseconds p50ProcessTime;
        std::chrono::nanoseconds p95ProcessTime;
        std::chrono::nanoseconds p99ProcessTime;
        size_t samplesProcessed;
    };

    MockPerformanceMonitor();
    ~MockPerformanceMonitor() = default;

    /**
     * @brief Record a processing event
     * @param processTime Time taken for processing
     * @param numSamples Number of samples processed
     * @param sampleRate Current sample rate
     */
    void recordProcessTime(std::chrono::nanoseconds processTime,
                          int numSamples,
                          double sampleRate);

    /**
     * @brief Simulate CPU load update
     */
    void updateCpuLoad(double load);

    /**
     * @brief Simulate X-run occurrence
     */
    void simulateXrun();

    /**
     * @brief Get current metrics
     */
    Metrics getMetrics() const noexcept;

    /**
     * @brief Reset all metrics
     */
    void reset();

private:
    mutable std::mutex mutex_;
    Metrics metrics_;
    std::vector<std::chrono::nanoseconds> processTimes_;
};

/**
 * @brief Test fixture utilities
 */
class TestUtils
{
public:
    /**
     * @brief Compare floating point values with tolerance
     */
    static bool approximatelyEqual(float a, float b, float tolerance = 1e-6f);
    static bool approximatelyEqual(double a, double b, double tolerance = 1e-12);

    /**
     * @brief Generate test audio buffer
     */
    static std::vector<float> generateSineWave(size_t length, double frequency,
                                             double sampleRate, float amplitude = 1.0f);

    /**
     * @brief Measure execution time of a function
     */
    template<typename Func>
    static auto measureExecutionTime(Func&& func)
    {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = func();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        return std::make_pair(result, duration);
    }

    /**
     * @brief Create temporary directory for tests
     */
    static std::string createTempDirectory();

    /**
     * @brief Clean up temporary directory
     */
    static void cleanupTempDirectory(const std::string& path);
};

} // namespace daw::core::testing
