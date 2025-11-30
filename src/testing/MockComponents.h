#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <random>
#include <vector>
#include <memory>

namespace cppmusic {
namespace testing {

/**
 * Mock Audio Buffer Generator for Testing
 *
 * Generates deterministic and controllable audio signals for unit testing
 * audio processing components without requiring actual audio hardware.
 *
 * Features:
 * - Multiple signal types (sine, noise, chirp, impulse, etc.)
 * - MIDI message generation with timing control
 * - Buffer underrun/overrun simulation
 * - Latency and jitter simulation
 * - Deterministic random seeds for reproducible tests
 */
class MockAudioBufferGenerator {
public:
    enum class SignalType {
        Silence = 0,
        Sine,
        WhiteNoise,
        PinkNoise,
        BrownNoise,
        Chirp,           // Frequency sweep
        ImpulseResponse, // Dirac delta
        SquareWave,
        SawtoothWave,
        TriangleWave,
        MultiTone,       // Multiple sine waves
        AudioFile        // From loaded file
    };

    struct GeneratorConfig {
        double sampleRate = 44100.0;
        int numChannels = 2;
        int blockSize = 512;
        SignalType signalType = SignalType::Sine;

        // Signal parameters
        float frequency = 440.0f;           // Primary frequency
        float amplitude = 0.5f;             // 0.0-1.0
        float phase = 0.0f;                 // Phase offset in radians
        std::vector<float> frequencies;     // For multi-tone
        std::vector<float> amplitudes;      // Per-frequency amplitudes

        // Noise parameters
        float noiseLevel = 0.1f;            // Added noise level
        uint32_t randomSeed = 12345;        // For reproducible noise

        // Sweep parameters (for chirp)
        float startFreq = 20.0f;
        float endFreq = 20000.0f;
        float sweepDuration = 1.0f;         // seconds

        // Timing simulation
        bool simulateLatency = false;
        int latencyMs = 10;                 // Simulated latency
        bool simulateJitter = false;
        float jitterMs = 1.0f;              // Max jitter

        // Reliability simulation
        bool simulateDropouts = false;
        float dropoutProbability = 0.001f;  // Per block
        bool simulateOverload = false;
        float overloadThreshold = 0.9f;
    };

    //==============================================================================
    explicit MockAudioBufferGenerator(const GeneratorConfig& config = {});
    ~MockAudioBufferGenerator() = default;

    // Non-copyable but movable
    MockAudioBufferGenerator(const MockAudioBufferGenerator&) = delete;
    MockAudioBufferGenerator& operator=(const MockAudioBufferGenerator&) = delete;
    MockAudioBufferGenerator(MockAudioBufferGenerator&&) = default;
    MockAudioBufferGenerator& operator=(MockAudioBufferGenerator&&) = default;

    //==============================================================================
    // Configuration
    void setConfig(const GeneratorConfig& config);
    const GeneratorConfig& getConfig() const { return config_; }

    void setSignalType(SignalType type);
    void setFrequency(float frequency);
    void setAmplitude(float amplitude);
    void setSampleRate(double sampleRate);
    void setBlockSize(int blockSize);
    void setRandomSeed(uint32_t seed);

    //==============================================================================
    // Audio Generation
    void fillBuffer(juce::AudioBuffer<float>& buffer);
    void fillBuffer(juce::AudioBuffer<double>& buffer);

    // Generate specific signal types
    void generateSine(juce::AudioBuffer<float>& buffer, float frequency, float amplitude);
    void generateNoise(juce::AudioBuffer<float>& buffer, SignalType noiseType, float amplitude);
    void generateChirp(juce::AudioBuffer<float>& buffer);
    void generateImpulse(juce::AudioBuffer<float>& buffer);
    void generateMultiTone(juce::AudioBuffer<float>& buffer);

    //==============================================================================
    // MIDI Generation
    struct MidiSequence {
        struct Event {
            double timeInSeconds = 0.0;
            juce::MidiMessage message;

            Event() = default;
            Event(double time, const juce::MidiMessage& msg) : timeInSeconds(time), message(msg) {}
        };

        std::vector<Event> events;
        double totalDuration = 0.0;
        bool looping = false;
    };

    void setMidiSequence(const MidiSequence& sequence);
    void generateMidiForBlock(juce::MidiBuffer& midiBuffer, int blockSize, double currentTime);

    // Convenience methods for common MIDI patterns
    MidiSequence createScaleSequence(int startNote = 60, int numNotes = 8, float noteDuration = 0.5f);
    MidiSequence createChordSequence(const std::vector<int>& notes, float chordDuration = 1.0f);
    MidiSequence createDrumPattern(int numBeats = 16, float beatDuration = 0.25f);

    //==============================================================================
    // Timing and Error Simulation
    void enableLatencySimulation(bool enabled, int latencyMs = 10);
    void enableJitterSimulation(bool enabled, float jitterMs = 1.0f);
    void enableDropoutSimulation(bool enabled, float probability = 0.001f);
    void enableOverloadSimulation(bool enabled, float threshold = 0.9f);

    // Trigger specific conditions for testing
    void triggerDropout();
    void triggerOverload();
    void triggerLatencySpike(int extraMs);

    //==============================================================================
    // Analysis and Verification
    struct AnalysisResult {
        float rmsLevel = 0.0f;
        float peakLevel = 0.0f;
        float dcOffset = 0.0f;
        float thd = 0.0f;               // Total Harmonic Distortion
        float snr = 0.0f;               // Signal-to-Noise Ratio
        bool hasClipping = false;
        bool hasDropouts = false;
        int numSamplesGenerated = 0;
    };

    AnalysisResult analyzeLastBuffer() const;
    void resetAnalysis();
    void enableAnalysis(bool enabled) { analysisEnabled_ = enabled; }

    //==============================================================================
    // File Loading (for AudioFile signal type)
    bool loadAudioFile(const juce::File& file);
    void setFileLooping(bool shouldLoop) { fileLooping_ = shouldLoop; }
    void setFilePosition(double positionInSeconds);

    //==============================================================================
    // Statistics
    struct Statistics {
        std::atomic<int> buffersGenerated{0};
        std::atomic<int> samplesGenerated{0};
        std::atomic<int> dropoutsTriggered{0};
        std::atomic<int> overloadsTriggered{0};
        std::atomic<float> averageAmplitude{0.0f};
        std::atomic<float> peakAmplitude{0.0f};
    };

    const Statistics& getStatistics() const { return stats_; }
    void resetStatistics();

private:
    //==============================================================================
    GeneratorConfig config_;

    //==============================================================================
    // Generation State
    double currentPhase_ = 0.0;
    double sweepPhase_ = 0.0;
    double currentTime_ = 0.0;
    std::mt19937 randomGenerator_;
    std::uniform_real_distribution<float> noiseDistribution_{-1.0f, 1.0f};

    //==============================================================================
    // Multi-tone state
    std::vector<double> tonePhases_;

    //==============================================================================
    // MIDI state
    MidiSequence midiSequence_;
    double midiTime_ = 0.0;
    int midiEventIndex_ = 0;

    //==============================================================================
    // Audio file state
    std::unique_ptr<juce::AudioFormatReader> audioFileReader_;
    juce::AudioBuffer<float> fileBuffer_;
    int filePosition_ = 0;
    bool fileLooping_ = true;

    //==============================================================================
    // Simulation state
    juce::Random jitterRandom_;
    bool nextBlockHasDropout_ = false;
    bool nextBlockHasOverload_ = false;
    int extraLatencyMs_ = 0;

    //==============================================================================
    // Analysis state
    bool analysisEnabled_ = false;
    AnalysisResult lastAnalysis_;

    //==============================================================================
    // Statistics
    Statistics stats_;

    //==============================================================================
    // Private methods
    template<typename FloatType>
    void fillBufferTemplate(juce::AudioBuffer<FloatType>& buffer);

    float generateSample(SignalType type, double phase, int channel);
    float generateNoiseSample(SignalType noiseType);
    void updatePhases(int numSamples);
    void applySimulatedConditions(juce::AudioBuffer<float>& buffer);
    void updateAnalysis(const juce::AudioBuffer<float>& buffer);

    // Pink noise filter state
    struct PinkNoiseFilter {
        std::array<float, 7> state{};
    } pinkNoiseFilter_;

    // Brown noise state
    float brownNoiseState_ = 0.0f;
};

//==============================================================================
/**
 * Mock Neural Inference Client for Testing AI Components
 *
 * Simulates the behavior of neural inference services without requiring
 * actual model files or GPU acceleration. Useful for testing AI-driven
 * audio processing components.
 */
class MockInferenceClient {
public:
    enum class ModelType {
        AudioClassification = 0,
        PitchDetection,
        OnsetDetection,
        TempoEstimation,
        SourceSeparation,
        EffectDetection,
        SynthGeneration,
        StyleTransfer
    };

    struct InferenceRequest {
        ModelType modelType = ModelType::AudioClassification;
        std::vector<float> inputFeatures;
        std::string modelId;
        float confidence = 1.0f;
        int batchSize = 1;
        bool requiresGPU = false;

        // Metadata
        double timestamp = 0.0;
        int requestId = 0;
    };

    struct InferenceResult {
        std::vector<float> outputProbabilities;
        std::vector<std::string> labels;
        float confidence = 0.0f;
        float processingTime = 0.0f; // ms
        bool success = true;
        std::string errorMessage;

        // Metadata
        int requestId = 0;
        double timestamp = 0.0;
    };

    struct MockConfig {
        float baseLatency = 5.0f;        // ms
        float latencyVariation = 2.0f;   // ms
        float successRate = 0.98f;       // 0.0-1.0
        float confidenceBase = 0.85f;    // Base confidence level
        float confidenceVariation = 0.1f; // Confidence variation
        bool simulateGPUAcceleration = true;
        float gpuSpeedup = 3.0f;         // GPU vs CPU speedup
        uint32_t randomSeed = 54321;
    };

    //==============================================================================
    explicit MockInferenceClient(const MockConfig& config = {});
    ~MockInferenceClient() = default;

    //==============================================================================
    // Configuration
    void setConfig(const MockConfig& config) { config_ = config; }
    const MockConfig& getConfig() const { return config_; }

    void setModelAvailable(ModelType type, bool available);
    void setModelAccuracy(ModelType type, float accuracy);
    void setBaseLatency(float latencyMs);
    void setSuccessRate(float rate);

    //==============================================================================
    // Inference Operations
    InferenceResult processRequest(const InferenceRequest& request);
    std::vector<InferenceResult> processBatch(const std::vector<InferenceRequest>& requests);

    // Async inference (returns immediately, result via callback)
    using InferenceCallback = std::function<void(const InferenceResult&)>;
    void processRequestAsync(const InferenceRequest& request, InferenceCallback callback);

    //==============================================================================
    // Model Management
    bool isModelLoaded(ModelType type) const;
    void loadModel(ModelType type, const std::string& modelPath);
    void unloadModel(ModelType type);
    std::vector<ModelType> getAvailableModels() const;

    //==============================================================================
    // Specialized Mock Responses
    InferenceResult createAudioClassificationResult(const std::vector<float>& features);
    InferenceResult createPitchDetectionResult(const std::vector<float>& features);
    InferenceResult createOnsetDetectionResult(const std::vector<float>& features);
    InferenceResult createTempoEstimationResult(const std::vector<float>& features);

    //==============================================================================
    // Error Simulation
    void simulateModelError(ModelType type, const std::string& errorMessage);
    void simulateGPUOutOfMemory();
    void simulateNetworkTimeout();
    void clearSimulatedErrors();

    //==============================================================================
    // Statistics
    struct Statistics {
        std::atomic<int> requestsProcessed{0};
        std::atomic<int> requestsFailed{0};
        std::atomic<float> averageLatency{0.0f};
        std::atomic<float> averageConfidence{0.0f};
        std::atomic<int> gpuRequests{0};
        std::atomic<int> cpuRequests{0};
    };

    const Statistics& getStatistics() const { return stats_; }
    void resetStatistics();

private:
    MockConfig config_;
    std::mt19937 randomGenerator_;
    std::map<ModelType, bool> availableModels_;
    std::map<ModelType, float> modelAccuracies_;
    std::map<ModelType, std::string> simulatedErrors_;
    Statistics stats_;

    // Generate realistic mock results based on model type
    std::vector<float> generateMockProbabilities(ModelType type, int numClasses);
    std::vector<std::string> getLabelsForModel(ModelType type);
    float calculateProcessingTime(const InferenceRequest& request);
};

//==============================================================================
/**
 * Mock Audio Device Manager for Testing Device I/O
 *
 * Simulates audio device behavior including device enumeration,
 * sample rate changes, buffer size changes, and device failures.
 */
class MockDeviceManager : public juce::AudioIODeviceCallback {
public:
    struct DeviceInfo {
        juce::String name;
        juce::String typeName;
        std::vector<double> availableSampleRates;
        std::vector<int> availableBufferSizes;
        int maxInputChannels = 0;
        int maxOutputChannels = 0;
        bool isDefault = false;
        bool isActive = false;
        float latency = 5.0f; // ms
    };

    struct MockDeviceConfig {
        std::vector<DeviceInfo> inputDevices;
        std::vector<DeviceInfo> outputDevices;
        bool simulateDeviceChanges = false;
        float deviceChangeInterval = 30.0f; // seconds
        bool simulateDeviceFailures = false;
        float failureProbability = 0.001f;
        bool simulateHotplug = false;
    };

    //==============================================================================
    explicit MockDeviceManager(const MockDeviceConfig& config = {});
    ~MockDeviceManager() override;

    //==============================================================================
    // AudioIODeviceCallback implementation
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                        int numInputChannels,
                                        float* const* outputChannelData,
                                        int numOutputChannels,
                                        int numSamples,
                                        const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    //==============================================================================
    // Device Management
    void addDevice(const DeviceInfo& device, bool isInput);
    void removeDevice(const juce::String& deviceName, bool isInput);
    void setDeviceActive(const juce::String& deviceName, bool active);

    std::vector<DeviceInfo> getInputDevices() const { return config_.inputDevices; }
    std::vector<DeviceInfo> getOutputDevices() const { return config_.outputDevices; }

    DeviceInfo* findDevice(const juce::String& name, bool isInput);

    //==============================================================================
    // Device State Simulation
    void simulateDeviceFailure(const juce::String& deviceName);
    void simulateDeviceReconnection(const juce::String& deviceName);
    void simulateHotplugEvent(const DeviceInfo& device, bool pluggedIn);
    void simulateSampleRateChange(double newSampleRate);
    void simulateBufferSizeChange(int newBufferSize);

    //==============================================================================
    // Audio Callback Registration
    void setAudioCallback(juce::AudioIODeviceCallback* callback) { audioCallback_ = callback; }
    void setBufferGenerator(MockAudioBufferGenerator* generator) { bufferGenerator_ = generator; }

    //==============================================================================
    // Current State
    double getCurrentSampleRate() const { return currentSampleRate_; }
    int getCurrentBufferSize() const { return currentBufferSize_; }
    bool isPlaying() const { return isPlaying_; }

    //==============================================================================
    // Statistics
    struct Statistics {
        std::atomic<int> callbacksProcessed{0};
        std::atomic<int> deviceFailures{0};
        std::atomic<int> hotplugEvents{0};
        std::atomic<float> averageCallbackTime{0.0f};
        std::atomic<int> underruns{0};
        std::atomic<int> overruns{0};
    };

    const Statistics& getStatistics() const { return stats_; }
    void resetStatistics();

private:
    MockDeviceConfig config_;
    juce::AudioIODeviceCallback* audioCallback_ = nullptr;
    MockAudioBufferGenerator* bufferGenerator_ = nullptr;

    // Current state
    double currentSampleRate_ = 44100.0;
    int currentBufferSize_ = 512;
    bool isPlaying_ = false;

    // Simulation state
    juce::Random random_;
    juce::Time lastDeviceChange_;
    std::map<juce::String, bool> deviceFailureStates_;

    Statistics stats_;

    // Audio buffers for simulation
    juce::AudioBuffer<float> inputBuffer_;
    juce::AudioBuffer<float> outputBuffer_;

    void updateDeviceStates();
    void processAudioCallback();

    // Create default devices
    void createDefaultDevices();
};

//==============================================================================
/**
 * Mock Performance Monitor for Testing Performance Critical Code
 *
 * Provides controllable performance metrics simulation for testing
 * performance monitoring and optimization systems.
 */
class MockPerformanceMonitor {
public:
    struct PerformanceProfile {
        std::string name;
        float baseCPUUsage = 0.3f;        // 0.0-1.0
        float cpuVariation = 0.1f;        // Variation around base
        float baseMemoryUsage = 0.4f;     // 0.0-1.0
        float memoryGrowthRate = 0.001f;  // Per operation
        float baseLatency = 5.0f;         // ms
        float latencyVariation = 1.0f;    // ms
        bool hasPeriodicSpikes = false;
        float spikeInterval = 10.0f;      // seconds
        float spikeMultiplier = 3.0f;
    };

    //==============================================================================
    explicit MockPerformanceMonitor(const PerformanceProfile& profile = {});
    ~MockPerformanceMonitor() = default;

    //==============================================================================
    // Profile Management
    void setProfile(const PerformanceProfile& profile) { profile_ = profile; }
    const PerformanceProfile& getProfile() const { return profile_; }

    void loadProfile(const std::string& profileName);
    void saveProfile(const std::string& profileName, const PerformanceProfile& profile);

    //==============================================================================
    // Performance Metrics
    float getCurrentCPUUsage();
    float getCurrentMemoryUsage();
    float getCurrentLatency();
    float getCurrentThroughput();

    // Trigger specific conditions
    void triggerCPUSpike(float multiplier = 3.0f, float durationSeconds = 1.0f);
    void triggerMemoryPressure(float targetUsage = 0.9f);
    void triggerLatencySpike(float additionalMs = 50.0f);
    void simulateGarbageCollection();

    //==============================================================================
    // System State Simulation
    void setSystemLoad(float load); // 0.0-1.0
    void setAvailableMemory(float availableGB);
    void setThermalState(float temperature); // Celsius
    void setBatteryLevel(float level); // 0.0-1.0

    //==============================================================================
    // Monitoring
    struct MonitoringData {
        double timestamp = 0.0;
        float cpuUsage = 0.0f;
        float memoryUsage = 0.0f;
        float latency = 0.0f;
        float throughput = 0.0f;
        float temperature = 0.0f;
        bool inSpike = false;
    };

    MonitoringData getCurrentData();
    std::vector<MonitoringData> getHistory(int numSamples = 100);
    void clearHistory();

    //==============================================================================
    // Preset Profiles
    static PerformanceProfile createLowEndProfile();
    static PerformanceProfile createMidRangeProfile();
    static PerformanceProfile createHighEndProfile();
    static PerformanceProfile createStressTestProfile();
    static PerformanceProfile createMobileProfile();

private:
    PerformanceProfile profile_;
    std::mt19937 randomGenerator_;

    // State tracking
    double startTime_;
    double lastSpikeTime_ = 0.0;
    float currentMemoryBase_;
    float systemLoad_ = 0.3f;
    float availableMemory_ = 8.0f; // GB
    float temperature_ = 50.0f; // Celsius
    float batteryLevel_ = 1.0f;

    // Spike state
    bool inCPUSpike_ = false;
    double cpuSpikeEndTime_ = 0.0;
    float cpuSpikeMultiplier_ = 1.0f;

    bool inLatencySpike_ = false;
    double latencySpikeEndTime_ = 0.0;
    float additionalLatency_ = 0.0f;

    // History
    std::vector<MonitoringData> history_;
    static constexpr int MAX_HISTORY_SIZE = 1000;

    // Static profile storage
    static std::map<std::string, PerformanceProfile> savedProfiles_;

    double getCurrentTime() const;
    bool shouldTriggerPeriodicSpike() const;
};

} // namespace testing
} // namespace cppmusic
