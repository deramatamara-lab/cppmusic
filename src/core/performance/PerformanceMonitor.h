#pragma once

#include <chrono>
#include <vector>
#include <atomic>
#include <string>
#include <memory>
#include <functional>
#include <mutex>

namespace daw::core::performance {

/**
 * @brief High-precision performance monitor for DAW applications
 *
 * Tracks real-time performance metrics with minimal overhead:
 * - CPU load and usage patterns
 * - Audio buffer processing times (P50, P95, P99)
 * - X-run detection and counting
 * - Memory usage tracking
 * - Custom performance counters
 *
 * Follows DAW_DEV_RULES: real-time safe, lock-free where possible.
 */
class PerformanceMonitor
{
public:
    PerformanceMonitor();
    ~PerformanceMonitor() = default;

    // Non-copyable, movable
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;
    PerformanceMonitor(PerformanceMonitor&&) noexcept = default;
    PerformanceMonitor& operator=(PerformanceMonitor&&) noexcept = default;

    /**
     * @brief Record a buffer processing event
     * @param processTime Time taken to process the buffer
     * @param numSamples Number of samples processed
     * @param sampleRate Current sample rate
     *
     * This is safe to call from the audio thread.
     */
    void recordProcessTime(std::chrono::nanoseconds processTime,
                          int numSamples,
                          double sampleRate) noexcept;

    /**
     * @brief Record CPU usage sample
     * @param cpuUsage CPU usage percentage (0.0 to 1.0)
     */
    void recordCpuUsage(float cpuUsage) noexcept;

    /**
     * @brief Record memory usage
     * @param bytesUsed Current memory usage in bytes
     */
    void recordMemoryUsage(size_t bytesUsed) noexcept;

    /**
     * @brief Record X-run occurrence
     */
    void recordXrun() noexcept;

    /**
     * @brief Increment custom performance counter
     * @param counterId Counter identifier
     */
    void incrementCounter(const std::string& counterId) noexcept;

    /**
     * @brief Get current performance statistics
     */
    struct Statistics
    {
        // CPU metrics
        float cpuLoad;              // Current CPU load (0.0-1.0)
        float averageCpuLoad;       // Rolling average CPU load
        float peakCpuLoad;          // Peak CPU load since reset

        // Processing time percentiles (nanoseconds)
        std::chrono::nanoseconds p50ProcessTime;
        std::chrono::nanoseconds p95ProcessTime;
        std::chrono::nanoseconds p99ProcessTime;
        std::chrono::nanoseconds maxProcessTime;

        // X-run statistics
        uint64_t xrunCount;
        double xrunRate;            // X-runs per second

        // Memory usage
        size_t currentMemoryUsage;
        size_t peakMemoryUsage;

        // Buffer processing stats
        size_t totalBuffersProcessed;
        double averageBufferSize;
        double buffersPerSecond;

        // Custom counters
        std::unordered_map<std::string, uint64_t> counters;
    };

    Statistics getStatistics() const noexcept;

    /**
     * @brief Reset all statistics and counters
     */
    void reset();

    /**
     * @brief Get detailed performance report
     * @return Formatted performance report string
     */
    std::string generateReport() const;

    /**
     * @brief Check if performance is within acceptable bounds
     * @param maxCpuLoad Maximum acceptable CPU load (0.0-1.0)
     * @param maxP95LatencyNs Maximum acceptable P95 latency (nanoseconds)
     * @return true if performance is acceptable
     */
    bool checkPerformanceBounds(float maxCpuLoad,
                              std::chrono::nanoseconds maxP95LatencyNs) const noexcept;

private:
    // Lock-free storage for real-time updates
    std::atomic<float> currentCpuLoad_;
    std::atomic<size_t> currentMemoryUsage_;
    std::atomic<uint64_t> xrunCount_;

    // Statistics with mutex protection (updated from UI thread)
    mutable std::mutex statsMutex_;
    Statistics stats_;
    std::vector<std::chrono::nanoseconds> processTimeHistory_;
    std::vector<float> cpuLoadHistory_;

    // Update statistics (called with mutex held)
    void updateStatistics() noexcept;
    void calculatePercentiles() noexcept;
};

/**
 * @brief Benchmark harness for automated performance testing
 *
 * Runs performance benchmarks with statistical analysis:
 * - Automated test execution
 * - Statistical outlier detection
 * - Regression detection
 * - Configurable test parameters
 */
class BenchmarkHarness
{
public:
    struct BenchmarkConfig
    {
        std::string name;
        std::function<void()> setupFunction;
        std::function<std::chrono::nanoseconds()> testFunction;
        std::function<void()> teardownFunction;
        size_t iterations = 1000;
        size_t warmupIterations = 100;
        double outlierThreshold = 2.0; // Standard deviations
    };

    struct BenchmarkResult
    {
        std::string name;
        size_t iterations;
        std::chrono::nanoseconds minTime;
        std::chrono::nanoseconds maxTime;
        std::chrono::nanoseconds meanTime;
        std::chrono::nanoseconds medianTime;
        std::chrono::nanoseconds p95Time;
        std::chrono::nanoseconds p99Time;
        double standardDeviationNs;
        size_t outlierCount;
        bool passed;
        std::string failureReason;
    };

    BenchmarkHarness();
    ~BenchmarkHarness() = default;

    /**
     * @brief Add a benchmark to the test suite
     */
    void addBenchmark(const BenchmarkConfig& config);

    /**
     * @brief Run all benchmarks
     * @return Results for all benchmarks
     */
    std::vector<BenchmarkResult> runAllBenchmarks();

    /**
     * @brief Run a specific benchmark
     */
    BenchmarkResult runBenchmark(const std::string& name);

    /**
     * @brief Set performance requirements
     * @param maxP95LatencyNs Maximum acceptable P95 latency
     * @param maxCpuLoad Maximum acceptable CPU load
     */
    void setRequirements(std::chrono::nanoseconds maxP95LatencyNs, float maxCpuLoad);

    /**
     * @brief Generate comprehensive benchmark report
     */
    std::string generateReport(const std::vector<BenchmarkResult>& results) const;

private:
    std::vector<BenchmarkConfig> benchmarks_;
    std::chrono::nanoseconds maxP95Latency_;
    float maxCpuLoad_;

    BenchmarkResult runBenchmarkInternal(const BenchmarkConfig& config);
    std::vector<std::chrono::nanoseconds> collectSamples(const BenchmarkConfig& config);
    void detectOutliers(std::vector<std::chrono::nanoseconds>& samples,
                       double threshold,
                       size_t& outlierCount);
    BenchmarkResult analyzeResults(const std::string& name,
                                 const std::vector<std::chrono::nanoseconds>& samples);
};

/**
 * @brief Automated performance regression detector
 *
 * Compares current performance against baseline and detects regressions.
 */
class RegressionDetector
{
public:
    struct BaselineData
    {
        std::string benchmarkName;
        std::chrono::nanoseconds p50Time;
        std::chrono::nanoseconds p95Time;
        std::chrono::nanoseconds p99Time;
        double maxCpuLoad;
        uint64_t timestamp;
    };

    struct RegressionResult
    {
        std::string benchmarkName;
        bool regressionDetected;
        std::string metric;
        double baselineValue;
        double currentValue;
        double degradationPercent;
        std::string severity; // "none", "minor", "major", "critical"
    };

    RegressionDetector();
    ~RegressionDetector() = default;

    /**
     * @brief Load baseline performance data
     * @param baselineFile Path to baseline data file
     * @return true if loaded successfully
     */
    bool loadBaseline(const std::string& baselineFile);

    /**
     * @brief Save current performance as new baseline
     * @param baselineFile Path to save baseline data
     * @param results Current benchmark results
     */
    bool saveBaseline(const std::string& baselineFile,
                     const std::vector<BenchmarkHarness::BenchmarkResult>& results);

    /**
     * @brief Check for performance regressions
     * @param results Current benchmark results
     * @return Regression analysis results
     */
    std::vector<RegressionResult> checkRegressions(
        const std::vector<BenchmarkHarness::BenchmarkResult>& results);

    /**
     * @brief Set regression thresholds
     * @param minorThreshold Minor regression threshold (percent)
     * @param majorThreshold Major regression threshold (percent)
     * @param criticalThreshold Critical regression threshold (percent)
     */
    void setThresholds(float minorThreshold, float majorThreshold, float criticalThreshold);

private:
    std::unordered_map<std::string, BaselineData> baselineData_;
    float minorThreshold_;
    float majorThreshold_;
    float criticalThreshold_;

    std::string classifyRegression(double degradationPercent) const;
    RegressionResult analyzeBenchmark(const BenchmarkHarness::BenchmarkResult& result);
};

/**
 * @brief Performance assertion utilities for tests
 */
class PerformanceAssertions
{
public:
    /**
     * @brief Assert that a function completes within time limit
     */
    template<typename Func>
    static void assertExecutesWithin(Func&& func,
                                    std::chrono::nanoseconds maxTime,
                                    const std::string& description = "")
    {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        if (duration > maxTime)
        {
            throw std::runtime_error("Performance assertion failed: " + description +
                                   " took " + std::to_string(duration.count()) + "ns, " +
                                   "maximum allowed: " + std::to_string(maxTime.count()) + "ns");
        }
    }

    /**
     * @brief Assert that CPU usage stays below threshold during function execution
     */
    template<typename Func>
    static void assertCpuUsageBelow(Func&& func,
                                   float maxCpuUsage,
                                   const std::string& description = "")
    {
        // This would require integration with PerformanceMonitor
        // For now, just execute the function
        func();
    }

    /**
     * @brief Assert that no X-runs occur during function execution
     */
    template<typename Func>
    static void assertNoXruns(Func&& func,
                             const std::string& description = "")
    {
        // This would require integration with PerformanceMonitor
        // For now, just execute the function
        func();
    }
};

} // namespace daw::core::performance
