#pragma once

#include "BenchmarkSystem.h"
#include <vector>
#include <memory>

namespace cppmusic::performance {

/**
 * Specialized benchmarks for audio processing components
 *
 * This class provides comprehensive performance testing for:
 * - AnalogModeledEQ processing
 * - Synthesizer voice rendering
 * - Effect chain processing
 * - Real-time parameter updates
 * - Memory allocation patterns
 * - SIMD optimization verification
 */

class AudioProcessingBenchmarks {
public:
    AudioProcessingBenchmarks();
    ~AudioProcessingBenchmarks() = default;

    // Core audio processing benchmarks
    void benchmarkEQProcessing();
    void benchmarkSynthesizerVoices();
    void benchmarkEffectChains();
    void benchmarkParameterUpdates();
    void benchmarkMemoryPatterns();
    void benchmarkSIMDOptimizations();

    // Comprehensive audio system benchmark
    void runFullAudioBenchmark();

    // Specific EQ performance tests
    struct EQBenchmarkResults {
        double processingTimeMs;
        double cpuUsagePercent;
        size_t memoryUsage;
        double filterStability;
        double dynamicRange;
        bool realtimeSafe;
    };

    EQBenchmarkResults benchmarkAnalogEQ(
        double sampleRate = 48000.0,
        int blockSize = 512,
        int numChannels = 2,
        bool enableOversampling = true,
        int numBands = 5
    );

    // Latency measurements
    struct LatencyMeasurement {
        double inputToOutputMs;
        double parameterUpdateMs;
        double worstCaseMs;
        double jitterMs;
    };

    LatencyMeasurement measureSystemLatency();

    // Load testing
    struct LoadTestResults {
        int maxSimultaneousVoices;
        double maxCPUBeforeDropouts;
        size_t maxMemoryUsage;
        double breakingPointMs;
    };

    LoadTestResults performLoadTest();

    // Automated regression testing for audio components
    void setupAudioRegressionTests();

private:
    std::unique_ptr<BenchmarkSystem> benchmarkSystem_;

    // Helper methods
    void setupTestEnvironment();
    void cleanupTestEnvironment();

    // Test data generation
    std::vector<float> generateTestSignal(int numSamples, double frequency, double sampleRate);
    std::vector<float> generateWhiteNoise(int numSamples, float amplitude = 0.5f);
    std::vector<float> generateComplexSignal(int numSamples, double sampleRate);

    // Signal analysis
    double calculateTHD(const std::vector<float>& signal, double fundamentalFreq, double sampleRate);
    double calculateSNR(const std::vector<float>& signal, const std::vector<float>& noise);
    double calculateDynamicRange(const std::vector<float>& signal);

    // Performance validation
    bool validateRealtimePerformance(double processingTimeMs, int blockSize, double sampleRate);
    bool validateMemoryConstraints(size_t memoryUsage, size_t maxAllowed);
};

/**
 * Real-time performance monitor for audio processing
 *
 * Provides continuous monitoring of audio system performance
 * with minimal overhead to avoid affecting the measurements
 */
class RealtimeAudioMonitor {
public:
    struct AudioPerformanceMetrics {
        double processingLoad;      // % of available time used
        double latency;            // ms
        int dropouts;             // count
        double cpuUsage;          // %
        size_t memoryUsage;       // bytes
        double temperature;       // CPU temp if available
        int activeVoices;
        double dynamicRange;      // dB
    };

    RealtimeAudioMonitor();
    ~RealtimeAudioMonitor();

    void start();
    void stop();

    // Record metrics from audio callback
    void recordProcessingTime(double timeMs);
    void recordDropout();
    void recordLatency(double latencyMs);
    void updateVoiceCount(int voices);

    // Get current metrics
    AudioPerformanceMetrics getCurrentMetrics() const;

    // Get historical data
    std::vector<AudioPerformanceMetrics> getHistory(size_t maxSamples = 1000) const;

    // Alert system
    using AlertCallback = std::function<void(const std::string& alertType, const std::string& message)>;
    void setAlertCallback(AlertCallback callback);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

/**
 * Automated CI/CD performance testing
 *
 * Provides automated performance testing suitable for continuous integration
 */
class ContinuousPerformanceTesting {
public:
    struct TestConfiguration {
        double maxAllowedLatency = 10.0;        // ms
        double maxAllowedCPU = 25.0;           // %
        size_t maxAllowedMemory = 100 * 1024 * 1024;  // 100MB
        double regressionThreshold = 5.0;      // %
        bool enableStressTests = false;
        bool enableLongRunningTests = false;
    };

    ContinuousPerformanceTesting(const TestConfiguration& config = {});

    // Run full CI test suite
    struct CITestResults {
        bool allTestsPassed;
        std::vector<std::string> failedTests;
        std::vector<std::string> warnings;
        double totalExecutionTimeMs;
        std::string detailedReport;
    };

    CITestResults runCITestSuite();

    // Individual test categories
    bool runQuickPerformanceTests();
    bool runMemoryLeakTests();
    bool runStabilityTests();
    bool runRegressionTests();

    // Generate CI-friendly reports
    void generateJUnitReport(const std::string& filename) const;
    void generateMarkdownReport(const std::string& filename) const;
    void generateMetricsFile(const std::string& filename) const;

private:
    TestConfiguration config_;
    std::vector<std::string> testResults_;
    std::chrono::high_resolution_clock::time_point startTime_;
};

/**
 * Performance optimization recommendations
 *
 * Analyzes performance data and provides optimization suggestions
 */
class PerformanceOptimizer {
public:
    struct OptimizationSuggestion {
        std::string category;      // "CPU", "Memory", "Latency", etc.
        std::string suggestion;    // Human-readable suggestion
        double potentialImprovement; // Estimated improvement %
        int priority;             // 1-5, 5 being highest
        bool autoApplicable;      // Can be applied automatically
    };

    std::vector<OptimizationSuggestion> analyzePerformance(
        const BenchmarkSystem::PerformanceReport& report);

    // Specific analyzers
    std::vector<OptimizationSuggestion> analyzeCPUUsage(const std::unordered_map<std::string, double>& metrics);
    std::vector<OptimizationSuggestion> analyzeMemoryUsage(size_t current, size_t peak);
    std::vector<OptimizationSuggestion> analyzeLatency(const std::unordered_map<std::string, double>& latencies);

    // Auto-optimization (where safe)
    bool applyOptimizations(const std::vector<OptimizationSuggestion>& suggestions);
};

// Global instances for easy access
extern std::unique_ptr<AudioProcessingBenchmarks> g_audioBenchmarks;
extern std::unique_ptr<RealtimeAudioMonitor> g_audioMonitor;

// Convenience functions
void initializeAudioBenchmarking();
void shutdownAudioBenchmarking();

// Macros for easy profiling in audio code
#define PROFILE_AUDIO_FUNCTION() \
    static auto* metrics = cppmusic::performance::getMetrics(__FUNCTION__); \
    cppmusic::performance::ScopedProfiler profiler(__FUNCTION__, *metrics)

#define PROFILE_AUDIO_BLOCK(name) \
    static auto* metrics = cppmusic::performance::getMetrics(name); \
    cppmusic::performance::ScopedProfiler profiler(name, *metrics)

} // namespace cppmusic::performance
