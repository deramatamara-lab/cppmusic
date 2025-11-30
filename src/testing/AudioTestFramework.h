#pragma once

#include "MockComponents.h"
#include "src/audio/AdvancedSynthesizer.h"
#include "src/audio/AnalogModeledEQ.h"
#include "src/audio/SpectralAnalysisEngine.h"
#include "src/audio/MasterAudioProcessor.h"
#include "src/core/EngineContext.h"
#include "src/core/RTMemoryPool.h"
#include "src/core/ServiceLocator.h"
#include <gtest/gtest.h>
#include <memory>
#include <chrono>

namespace cppmusic {
namespace testing {

/**
 * Comprehensive Audio Testing Framework
 *
 * Provides a complete testing environment for audio processing components
 * using mock objects and controlled test conditions. Enables automated
 * testing of real-time audio systems without hardware dependencies.
 *
 * Features:
 * - Automated audio quality analysis
 * - Performance regression testing
 * - Real-time constraint validation
 * - Comprehensive test scenarios
 * - CI/CD integration support
 */
class AudioTestFramework {
public:
    //==============================================================================
    // Test Configuration
    struct TestConfig {
        double sampleRate = 44100.0;
        int blockSize = 512;
        int numChannels = 2;
        float testDuration = 1.0f;       // seconds
        float tolerance = 0.001f;        // Error tolerance
        bool enablePerformanceTesting = true;
        bool enableQualityAnalysis = true;
        bool enableRealtimeValidation = true;

        // Test limits
        float maxCPUUsage = 0.7f;        // 70%
        float maxLatency = 20.0f;        // ms
        float maxMemoryUsage = 100.0f;   // MB
        float minSNR = 60.0f;            // dB

        // Random testing parameters
        uint32_t randomSeed = 12345;
        int numRandomTests = 100;
        bool enableFuzzTesting = false;
    };

    //==============================================================================
    // Test Results
    struct TestResult {
        bool passed = false;
        std::string testName;
        std::string category;

        // Performance metrics
        float maxCPUUsage = 0.0f;
        float averageCPUUsage = 0.0f;
        float maxLatency = 0.0f;
        float averageLatency = 0.0f;
        float memoryUsage = 0.0f;

        // Audio quality metrics
        float snr = 0.0f;                // Signal-to-Noise Ratio
        float thd = 0.0f;                // Total Harmonic Distortion
        float dynamicRange = 0.0f;
        float phaseCoherence = 0.0f;

        // Realtime safety
        bool hadRealtimeViolations = false;
        int heapAllocations = 0;
        int lockContention = 0;

        // Error information
        std::vector<std::string> errors;
        std::vector<std::string> warnings;

        // Timing
        std::chrono::milliseconds executionTime{0};
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point endTime;
    };

    //==============================================================================
    // Test Suite Results
    struct TestSuiteResult {
        std::string suiteName;
        std::vector<TestResult> results;
        int totalTests = 0;
        int passedTests = 0;
        int failedTests = 0;
        std::chrono::milliseconds totalExecutionTime{0};

        // Summary statistics
        float averageCPUUsage = 0.0f;
        float maxCPUUsage = 0.0f;
        float averageLatency = 0.0f;
        float maxLatency = 0.0f;
        float averageSNR = 0.0f;
        float minSNR = std::numeric_limits<float>::max();

        bool hasRealtimeViolations = false;
        int totalHeapAllocations = 0;

        // Generate report
        std::string generateReport() const;
        void exportToJUnit(const std::string& filename) const;
        void exportToJSON(const std::string& filename) const;
    };

    //==============================================================================
    explicit AudioTestFramework(const TestConfig& config = {});
    ~AudioTestFramework();

    // Non-copyable, non-movable
    AudioTestFramework(const AudioTestFramework&) = delete;
    AudioTestFramework& operator=(const AudioTestFramework&) = delete;
    AudioTestFramework(AudioTestFramework&&) = delete;
    AudioTestFramework& operator=(AudioTestFramework&&) = delete;

    //==============================================================================
    // Configuration
    void setConfig(const TestConfig& config) { config_ = config; }
    const TestConfig& getConfig() const { return config_; }

    void setSampleRate(double sampleRate);
    void setBlockSize(int blockSize);
    void setTestDuration(float seconds);
    void setTolerances(float errorTolerance, float snrMinimum);

    //==============================================================================
    // Test Execution
    TestSuiteResult runAllTests();
    TestSuiteResult runSynthesizerTests();
    TestSuiteResult runEQTests();
    TestSuiteResult runSpectralAnalysisTests();
    TestSuiteResult runMasterProcessorTests();
    TestSuiteResult runPerformanceTests();
    TestSuiteResult runRealtimeTests();
    TestSuiteResult runStressTests();

    // Individual test methods
    TestResult testSynthesizerBasicOperation();
    TestResult testSynthesizerPolyphony();
    TestResult testSynthesizerMPE();
    TestResult testSynthesizerQuantumMode();
    TestResult testSynthesizerNeuralMode();
    TestResult testSynthesizerFractalMode();

    TestResult testEQFrequencyResponse();
    TestResult testEQAnalogModeling();
    TestResult testEQVintageEmulations();
    TestResult testEQPerformance();

    TestResult testSpectralAnalysisAccuracy();
    TestResult testSpectralAnalysisRealtimePerformance();
    TestResult testSpectralAnalysisMLFeatures();
    TestResult testPitchDetection();
    TestResult testOnsetDetection();
    TestResult testTempoEstimation();

    TestResult testMasterProcessorIntegration();
    TestResult testMasterProcessorLatency();
    TestResult testMasterProcessorOverload();
    TestResult testEmergencyProtection();

    //==============================================================================
    // Specialized Testing
    TestResult performFrequencyResponseTest(audio::AnalogModeledEQ& eq,
                                          float startFreq, float endFreq, int numPoints);
    TestResult performPolyphonyStressTest(audio::AdvancedSynthesizer& synth,
                                        int maxVoices, float duration);
    TestResult performLatencyMeasurement(audio::MasterAudioProcessor& processor);
    TestResult performRealtimeSafetyAudit(audio::MasterAudioProcessor& processor);

    //==============================================================================
    // Audio Quality Analysis
    struct AudioQualityMetrics {
        float snr = 0.0f;
        float thd = 0.0f;
        float thdPlusNoise = 0.0f;
        float dynamicRange = 0.0f;
        float frequencyResponse = 0.0f;  // Flatness measure
        float phaseLinearity = 0.0f;
        float stereoImaging = 0.0f;
        float groupDelay = 0.0f;
        bool hasAliasing = false;
        bool hasClipping = false;
        float crestFactor = 0.0f;
    };

    AudioQualityMetrics analyzeAudioQuality(const juce::AudioBuffer<float>& reference,
                                           const juce::AudioBuffer<float>& processed);

    //==============================================================================
    // Performance Benchmarking
    struct BenchmarkResult {
        std::string componentName;
        float processingTime = 0.0f;     // ms per block
        float cpuUsage = 0.0f;           // 0.0-1.0
        float memoryUsage = 0.0f;        // MB
        float throughput = 0.0f;         // blocks per second
        int maxPolyphony = 0;            // Voices before overload

        // Scaling characteristics
        std::vector<float> polyphonyCurve;     // CPU vs voice count
        std::vector<float> blockSizeCurve;     // CPU vs block size
        std::vector<float> sampleRateCurve;    // CPU vs sample rate
    };

    BenchmarkResult benchmarkComponent(const std::string& componentName,
                                     std::function<void(juce::AudioBuffer<float>&,
                                                      const juce::MidiBuffer&)> processor);

    //==============================================================================
    // Realtime Safety Validation
    struct RealtimeSafetyReport {
        bool isRealtimeSafe = true;
        int heapAllocations = 0;
        int systemCalls = 0;
        int lockOperations = 0;
        int fileOperations = 0;
        int networkOperations = 0;
        std::vector<std::string> violations;

        // Timing analysis
        float maxProcessingTime = 0.0f;
        float averageProcessingTime = 0.0f;
        float processingTimeVariance = 0.0f;
        bool hasTimeoutViolations = false;
    };

    RealtimeSafetyReport validateRealtimeSafety(
        std::function<void(juce::AudioBuffer<float>&, const juce::MidiBuffer&)> processor,
        int numIterations = 1000);

    //==============================================================================
    // Test Data Generation
    juce::AudioBuffer<float> generateTestSignal(MockAudioBufferGenerator::SignalType type,
                                               float frequency = 440.0f,
                                               float amplitude = 0.5f,
                                               float duration = 1.0f);

    juce::MidiBuffer generateTestMIDI(const std::vector<int>& notes,
                                     float noteDuration = 0.5f,
                                     int velocity = 100);

    //==============================================================================
    // Mock Component Access
    MockAudioBufferGenerator& getBufferGenerator() { return *bufferGenerator_; }
    MockInferenceClient& getInferenceClient() { return *inferenceClient_; }
    MockDeviceManager& getDeviceManager() { return *deviceManager_; }
    MockPerformanceMonitor& getPerformanceMonitor() { return *performanceMonitor_; }

    //==============================================================================
    // Test Environment
    void setupTestEnvironment();
    void teardownTestEnvironment();
    void resetTestState();

    // Controlled conditions
    void simulateHighCPULoad(float cpuUsage = 0.8f);
    void simulateLowMemory(float availableGB = 1.0f);
    void simulateDeviceLatency(int latencyMs = 50);
    void simulateDropouts(float probability = 0.01f);

    //==============================================================================
    // Regression Testing
    struct RegressionBaseline {
        std::string version;
        std::map<std::string, float> performanceMetrics;
        std::map<std::string, AudioQualityMetrics> qualityMetrics;
        std::chrono::system_clock::time_point timestamp;
    };

    void saveRegressionBaseline(const std::string& version, const TestSuiteResult& results);
    bool loadRegressionBaseline(const std::string& version);
    TestResult compareAgainstBaseline(const TestSuiteResult& currentResults);

    //==============================================================================
    // CI/CD Integration
    void enableContinuousIntegration(bool enabled) { ciMode_ = enabled; }
    void setOutputDirectory(const std::string& directory) { outputDirectory_ = directory; }
    void generateCoverageReport(const std::string& filename);
    int getExitCode() const; // 0 = all tests passed, non-zero = failures

private:
    //==============================================================================
    TestConfig config_;

    //==============================================================================
    // Mock Components
    std::unique_ptr<MockAudioBufferGenerator> bufferGenerator_;
    std::unique_ptr<MockInferenceClient> inferenceClient_;
    std::unique_ptr<MockDeviceManager> deviceManager_;
    std::unique_ptr<MockPerformanceMonitor> performanceMonitor_;

    //==============================================================================
    // Core Components for Testing
    std::unique_ptr<core::RTMemoryPool> memoryPool_;
    std::unique_ptr<core::EngineContext> engineContext_;
    std::unique_ptr<core::ServiceLocator> serviceLocator_;

    //==============================================================================
    // Test State
    bool environmentSetup_ = false;
    bool ciMode_ = false;
    std::string outputDirectory_ = "./test_results";

    // Current test context
    TestResult* currentTest_ = nullptr;
    std::chrono::system_clock::time_point testStartTime_;

    //==============================================================================
    // Regression Testing
    std::map<std::string, RegressionBaseline> regressionBaselines_;

    //==============================================================================
    // Private Test Utilities
    void startTest(const std::string& testName, const std::string& category);
    void endTest(bool passed, const std::vector<std::string>& errors = {});
    void recordPerformanceMetric(const std::string& metric, float value);
    void recordQualityMetric(const std::string& metric, float value);

    //==============================================================================
    // Audio Analysis Utilities
    float calculateSNR(const juce::AudioBuffer<float>& signal,
                      const juce::AudioBuffer<float>& noise);
    float calculateTHD(const juce::AudioBuffer<float>& buffer, float fundamentalFreq);
    float calculateDynamicRange(const juce::AudioBuffer<float>& buffer);
    float measureFrequencyResponse(const juce::AudioBuffer<float>& input,
                                  const juce::AudioBuffer<float>& output,
                                  float frequency);

    //==============================================================================
    // Performance Monitoring
    void startPerformanceMonitoring();
    void stopPerformanceMonitoring();
    float getCurrentCPUUsage() const;
    float getCurrentMemoryUsage() const;

    //==============================================================================
    // Realtime Safety Monitoring
    class RealtimeSafetyChecker {
    public:
        RealtimeSafetyChecker();
        ~RealtimeSafetyChecker();

        void startMonitoring();
        void stopMonitoring();
        RealtimeSafetyReport getReport() const;

    private:
        // Implementation details for monitoring heap allocations,
        // system calls, and timing violations
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };

    std::unique_ptr<RealtimeSafetyChecker> safetyChecker_;

    //==============================================================================
    // Test Execution Utilities
    template<typename ComponentType, typename... Args>
    TestResult executeComponentTest(const std::string& testName,
                                   const std::string& category,
                                   std::function<bool(ComponentType&, Args...)> testFunction,
                                   Args... args);

    void validateTestPreconditions();
    void validateTestPostconditions();
    bool withinTolerance(float expected, float actual, float tolerance) const;

    //==============================================================================
    // Report Generation
    void generateDetailedReport(const TestSuiteResult& results,
                               const std::string& filename);
    void generatePerformanceReport(const std::vector<BenchmarkResult>& benchmarks,
                                  const std::string& filename);
    void generateQualityReport(const std::map<std::string, AudioQualityMetrics>& metrics,
                              const std::string& filename);

    //==============================================================================
    // Static Test Data
    static std::map<std::string, std::vector<float>> getTestFrequencies();
    static std::vector<int> getTestMIDINotes();
    static std::vector<MockAudioBufferGenerator::SignalType> getTestSignalTypes();
};

//==============================================================================
/**
 * Google Test Fixtures for Audio Components
 *
 * Provides standardized test fixtures that can be used with Google Test
 * for systematic testing of audio components.
 */
class AudioComponentTest : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    // Test framework access
    std::unique_ptr<AudioTestFramework> testFramework_;
    AudioTestFramework::TestConfig config_;

    // Common test utilities
    void expectAudioQuality(const juce::AudioBuffer<float>& buffer, float minSNR = 60.0f);
    void expectRealtimeSafety(std::function<void()> operation);
    void expectPerformanceWithinLimits(std::function<void()> operation,
                                      float maxCPUPercent = 70.0f);
};

class SynthesizerTest : public AudioComponentTest {
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<audio::AdvancedSynthesizer> synthesizer_;
    std::unique_ptr<core::EngineContext> engineContext_;
    std::unique_ptr<core::RTMemoryPool> memoryPool_;
};

class EQTest : public AudioComponentTest {
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<audio::AnalogModeledEQ> eq_;
    std::unique_ptr<core::EngineContext> engineContext_;
    std::unique_ptr<core::RTMemoryPool> memoryPool_;
};

class SpectralAnalysisTest : public AudioComponentTest {
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<audio::SpectralAnalysisEngine> spectralAnalysis_;
    std::unique_ptr<core::EngineContext> engineContext_;
    std::unique_ptr<core::RTMemoryPool> memoryPool_;
};

class MasterProcessorTest : public AudioComponentTest {
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<audio::MasterAudioProcessor> masterProcessor_;
    std::unique_ptr<core::EngineContext> engineContext_;
    std::unique_ptr<core::RTMemoryPool> memoryPool_;
    std::unique_ptr<core::ServiceLocator> serviceLocator_;
};

} // namespace testing
} // namespace cppmusic

//==============================================================================
// Convenience Macros for Audio Testing
//==============================================================================

#define EXPECT_AUDIO_QUALITY(buffer, minSNR) \
    do { \
        auto metrics = testFramework_->analyzeAudioQuality(juce::AudioBuffer<float>(), buffer); \
        EXPECT_GE(metrics.snr, minSNR) << "Audio quality below threshold"; \
        EXPECT_FALSE(metrics.hasClipping) << "Audio has clipping"; \
        EXPECT_FALSE(metrics.hasAliasing) << "Audio has aliasing"; \
    } while(0)

#define EXPECT_REALTIME_SAFE(operation) \
    do { \
        auto report = testFramework_->validateRealtimeSafety([&](auto& buffer, auto& midi) { \
            operation; \
        }); \
        EXPECT_TRUE(report.isRealtimeSafe) << "Operation is not realtime safe"; \
        EXPECT_EQ(report.heapAllocations, 0) << "Heap allocations detected"; \
        EXPECT_EQ(report.lockOperations, 0) << "Lock operations detected"; \
    } while(0)

#define EXPECT_PERFORMANCE_WITHIN_LIMITS(operation, maxCPUPercent) \
    do { \
        auto result = testFramework_->benchmarkComponent("test", [&](auto& buffer, auto& midi) { \
            operation; \
        }); \
        EXPECT_LE(result.cpuUsage, maxCPUPercent / 100.0f) \
            << "CPU usage " << (result.cpuUsage * 100.0f) << "% exceeds limit " << maxCPUPercent << "%"; \
    } while(0)

#define EXPECT_LATENCY_WITHIN_LIMITS(processor, maxLatencyMs) \
    do { \
        auto result = testFramework_->performLatencyMeasurement(processor); \
        EXPECT_TRUE(result.passed) << "Latency measurement failed"; \
        EXPECT_LE(result.maxLatency, maxLatencyMs) \
            << "Latency " << result.maxLatency << "ms exceeds limit " << maxLatencyMs << "ms"; \
    } while(0)
