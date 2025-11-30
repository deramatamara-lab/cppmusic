#include "PerformanceMonitor.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace daw::core::performance {

PerformanceMonitor::PerformanceMonitor()
    : currentCpuLoad_(0.0f)
    , currentMemoryUsage_(0)
    , xrunCount_(0)
    , stats_{}
{}

void PerformanceMonitor::recordProcessTime(std::chrono::nanoseconds processTime,
                                         int numSamples,
                                         double sampleRate) noexcept
{
    // Lock-free update of counters
    stats_.totalBuffersProcessed.fetch_add(1, std::memory_order_relaxed);
    stats_.averageBufferSize.store(
        (stats_.averageBufferSize.load(std::memory_order_relaxed) * (stats_.totalBuffersProcessed.load(std::memory_order_relaxed) - 1) + numSamples) /
        stats_.totalBuffersProcessed.load(std::memory_order_relaxed),
        std::memory_order_relaxed);

    // Thread-safe update of history (with mutex)
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        processTimeHistory_.push_back(processTime);

        // Keep history bounded (last 1000 samples)
        if (processTimeHistory_.size() > 1000)
        {
            processTimeHistory_.erase(processTimeHistory_.begin(),
                                     processTimeHistory_.begin() + 100);
        }

        updateStatistics();
    }
}

void PerformanceMonitor::recordCpuUsage(float cpuUsage) noexcept
{
    currentCpuLoad_.store(cpuUsage, std::memory_order_release);

    std::lock_guard<std::mutex> lock(statsMutex_);
    cpuLoadHistory_.push_back(cpuUsage);

    // Keep history bounded
    if (cpuLoadHistory_.size() > 100)
    {
        cpuLoadHistory_.erase(cpuLoadHistory_.begin(),
                            cpuLoadHistory_.begin() + 10);
    }

    updateStatistics();
}

void PerformanceMonitor::recordMemoryUsage(size_t bytesUsed) noexcept
{
    currentMemoryUsage_.store(bytesUsed, std::memory_order_release);

    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_.currentMemoryUsage = bytesUsed;
    stats_.peakMemoryUsage = std::max(stats_.peakMemoryUsage, bytesUsed);
}

void PerformanceMonitor::recordXrun() noexcept
{
    xrunCount_.fetch_add(1, std::memory_order_relaxed);
}

void PerformanceMonitor::incrementCounter(const std::string& counterId) noexcept
{
    std::lock_guard<std::mutex> lock(statsMutex_);
    ++stats_.counters[counterId];
}

PerformanceMonitor::Statistics PerformanceMonitor::getStatistics() const noexcept
{
    Statistics result;

    // Copy atomic values
    result.cpuLoad = currentCpuLoad_.load(std::memory_order_acquire);
    result.xrunCount = xrunCount_.load(std::memory_order_acquire);

    // Copy protected values
    std::lock_guard<std::mutex> lock(statsMutex_);
    result.averageCpuLoad = stats_.averageCpuLoad;
    result.peakCpuLoad = stats_.peakCpuLoad;
    result.p50ProcessTime = stats_.p50ProcessTime;
    result.p95ProcessTime = stats_.p95ProcessTime;
    result.p99ProcessTime = stats_.p99ProcessTime;
    result.maxProcessTime = stats_.maxProcessTime;
    result.xrunRate = stats_.xrunRate;
    result.currentMemoryUsage = stats_.currentMemoryUsage;
    result.peakMemoryUsage = stats_.peakMemoryUsage;
    result.totalBuffersProcessed = stats_.totalBuffersProcessed.load(std::memory_order_relaxed);
    result.averageBufferSize = stats_.averageBufferSize.load(std::memory_order_relaxed);
    result.buffersPerSecond = stats_.buffersPerSecond;
    result.counters = stats_.counters;

    return result;
}

void PerformanceMonitor::reset()
{
    std::lock_guard<std::mutex> lock(statsMutex_);

    currentCpuLoad_.store(0.0f, std::memory_order_release);
    currentMemoryUsage_.store(0, std::memory_order_release);
    xrunCount_.store(0, std::memory_order_release);

    stats_ = Statistics{};
    processTimeHistory_.clear();
    cpuLoadHistory_.clear();
}

std::string PerformanceMonitor::generateReport() const
{
    const auto stats = getStatistics();

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    oss << "=== Performance Monitor Report ===\n";
    oss << "CPU Load: " << (stats.cpuLoad * 100.0f) << "% "
        << "(avg: " << (stats.averageCpuLoad * 100.0f) << "%, "
        << "peak: " << (stats.peakCpuLoad * 100.0f) << "%)\n";

    oss << "Process Times (ns):\n";
    oss << "  P50: " << stats.p50ProcessTime.count() << "\n";
    oss << "  P95: " << stats.p95ProcessTime.count() << "\n";
    oss << "  P99: " << stats.p99ProcessTime.count() << "\n";
    oss << "  Max: " << stats.maxProcessTime.count() << "\n";

    oss << "X-runs: " << stats.xrunCount << " (rate: " << stats.xrunRate << "/sec)\n";

    oss << "Memory: " << stats.currentMemoryUsage << " bytes "
        << "(peak: " << stats.peakMemoryUsage << " bytes)\n";

    oss << "Buffers: " << stats.totalBuffersProcessed << " processed "
        << "(avg size: " << stats.averageBufferSize << ", "
        << stats.buffersPerSecond << "/sec)\n";

    if (!stats.counters.empty())
    {
        oss << "Custom Counters:\n";
        for (const auto& [name, count] : stats.counters)
        {
            oss << "  " << name << ": " << count << "\n";
        }
    }

    return oss.str();
}

bool PerformanceMonitor::checkPerformanceBounds(float maxCpuLoad,
                                              std::chrono::nanoseconds maxP95LatencyNs) const noexcept
{
    const auto stats = getStatistics();
    return stats.cpuLoad <= maxCpuLoad && stats.p95ProcessTime <= maxP95LatencyNs;
}

void PerformanceMonitor::updateStatistics() noexcept
{
    if (!cpuLoadHistory_.empty())
    {
        stats_.averageCpuLoad = std::accumulate(cpuLoadHistory_.begin(),
                                              cpuLoadHistory_.end(), 0.0f) /
                               static_cast<float>(cpuLoadHistory_.size());
        stats_.peakCpuLoad = *std::max_element(cpuLoadHistory_.begin(),
                                             cpuLoadHistory_.end());
    }

    calculatePercentiles();

    // Calculate X-run rate (simplified)
    if (stats_.totalBuffersProcessed > 0)
    {
        stats_.xrunRate = static_cast<double>(stats_.xrunCount) /
                         (stats_.totalBuffersProcessed * 0.01); // Assuming 100Hz callback rate
    }

    // Calculate buffers per second
    stats_.buffersPerSecond = stats_.totalBuffersProcessed * 100.0; // Assuming 10ms buffers
}

void PerformanceMonitor::calculatePercentiles() noexcept
{
    if (processTimeHistory_.empty())
    {
        return;
    }

    std::vector<std::chrono::nanoseconds> sortedTimes = processTimeHistory_;
    std::sort(sortedTimes.begin(), sortedTimes.end());

    const size_t n = sortedTimes.size();
    stats_.maxProcessTime = sortedTimes.back();

    // Calculate percentiles
    auto getPercentile = [&](float percentile) -> std::chrono::nanoseconds {
        const float index = percentile * (n - 1) / 100.0f;
        const size_t lower = static_cast<size_t>(index);
        const size_t upper = std::min(lower + 1, n - 1);
        const float fraction = index - lower;

        const auto lowerTime = sortedTimes[lower].count();
        const auto upperTime = sortedTimes[upper].count();
        const auto interpolated = lowerTime + fraction * (upperTime - lowerTime);

        return std::chrono::nanoseconds(static_cast<long long>(interpolated));
    };

    stats_.p50ProcessTime = getPercentile(50.0f);
    stats_.p95ProcessTime = getPercentile(95.0f);
    stats_.p99ProcessTime = getPercentile(99.0f);
}

//==============================================================================

BenchmarkHarness::BenchmarkHarness()
    : maxP95Latency_(std::chrono::milliseconds(50))
    , maxCpuLoad_(0.8f)
{}

void BenchmarkHarness::addBenchmark(const BenchmarkConfig& config)
{
    benchmarks_.push_back(config);
}

std::vector<BenchmarkHarness::BenchmarkResult> BenchmarkHarness::runAllBenchmarks()
{
    std::vector<BenchmarkResult> results;
    results.reserve(benchmarks_.size());

    for (const auto& config : benchmarks_)
    {
        results.push_back(runBenchmarkInternal(config));
    }

    return results;
}

BenchmarkHarness::BenchmarkResult BenchmarkHarness::runBenchmark(const std::string& name)
{
    auto it = std::find_if(benchmarks_.begin(), benchmarks_.end(),
                          [&name](const BenchmarkConfig& config) {
                              return config.name == name;
                          });

    if (it == benchmarks_.end())
    {
        return BenchmarkResult{name, 0, {}, {}, {}, {}, {}, {}, 0.0, 0, false, "Benchmark not found"};
    }

    return runBenchmarkInternal(*it);
}

void BenchmarkHarness::setRequirements(std::chrono::nanoseconds maxP95LatencyNs, float maxCpuLoad)
{
    maxP95Latency_ = maxP95LatencyNs;
    maxCpuLoad_ = maxCpuLoad;
}

std::string BenchmarkHarness::generateReport(const std::vector<BenchmarkResult>& results) const
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    oss << "=== Benchmark Report ===\n";
    oss << "Total benchmarks: " << results.size() << "\n\n";

    size_t passed = 0;
    for (const auto& result : results)
    {
        if (result.passed) ++passed;

        oss << "Benchmark: " << result.name << "\n";
        oss << "  Iterations: " << result.iterations << "\n";
        oss << "  Min time: " << result.minTime.count() << " ns\n";
        oss << "  Max time: " << result.maxTime.count() << " ns\n";
        oss << "  Mean time: " << result.meanTime.count() << " ns\n";
        oss << "  Median time: " << result.medianTime.count() << " ns\n";
        oss << "  P95 time: " << result.p95Time.count() << " ns\n";
        oss << "  P99 time: " << result.p99Time.count() << " ns\n";
        oss << "  Std dev: " << result.standardDeviationNs << " ns\n";
        oss << "  Outliers: " << result.outlierCount << "\n";
        oss << "  Status: " << (result.passed ? "PASSED" : "FAILED");

        if (!result.passed)
        {
            oss << " - " << result.failureReason;
        }
        oss << "\n\n";
    }

    oss << "Summary: " << passed << "/" << results.size() << " benchmarks passed\n";

    const double passRate = static_cast<double>(passed) / results.size() * 100.0;
    oss << "Pass rate: " << passRate << "%\n";

    return oss.str();
}

BenchmarkHarness::BenchmarkResult BenchmarkHarness::runBenchmarkInternal(const BenchmarkConfig& config)
{
    // Setup
    if (config.setupFunction)
    {
        config.setupFunction();
    }

    // Warmup
    for (size_t i = 0; i < config.warmupIterations; ++i)
    {
        if (config.testFunction)
        {
            config.testFunction();
        }
    }

    // Collect samples
    auto samples = collectSamples(config);
    detectOutliers(samples, config.outlierThreshold, const_cast<size_t&>(
        const_cast<BenchmarkResult*>(nullptr)->outlierCount)); // Hack for now

    // Analyze results
    auto result = analyzeResults(config.name, samples);

    // Check requirements
    result.passed = result.p95Time <= maxP95Latency_;
    if (!result.passed)
    {
        result.failureReason = "P95 latency exceeds requirement: " +
                              std::to_string(result.p95Time.count()) + "ns > " +
                              std::to_string(maxP95Latency_.count()) + "ns";
    }

    // Teardown
    if (config.teardownFunction)
    {
        config.teardownFunction();
    }

    return result;
}

std::vector<std::chrono::nanoseconds> BenchmarkHarness::collectSamples(const BenchmarkConfig& config)
{
    std::vector<std::chrono::nanoseconds> samples;
    samples.reserve(config.iterations);

    for (size_t i = 0; i < config.iterations; ++i)
    {
        if (config.testFunction)
        {
            const auto start = std::chrono::high_resolution_clock::now();
            const auto duration = config.testFunction();
            const auto end = std::chrono::high_resolution_clock::now();

            // Use measured time if available, otherwise calculate
            const auto measured = (duration.count() > 0) ? duration :
                                std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            samples.push_back(measured);
        }
    }

    return samples;
}

void BenchmarkHarness::detectOutliers(std::vector<std::chrono::nanoseconds>& samples,
                                    double threshold,
                                    size_t& outlierCount)
{
    if (samples.size() < 3)
    {
        outlierCount = 0;
        return;
    }

    // Calculate mean and standard deviation
    const double sum = std::accumulate(samples.begin(), samples.end(), 0.0,
                                     [](double acc, std::chrono::nanoseconds val) {
                                         return acc + val.count();
                                     });
    const double mean = sum / samples.size();

    const double variance = std::accumulate(samples.begin(), samples.end(), 0.0,
                                          [mean](double acc, std::chrono::nanoseconds val) {
                                              const double diff = val.count() - mean;
                                              return acc + diff * diff;
                                          }) / samples.size();
    const double stdDev = std::sqrt(variance);

    // Remove outliers
    outlierCount = 0;
    samples.erase(std::remove_if(samples.begin(), samples.end(),
                               [mean, stdDev, threshold, &outlierCount](std::chrono::nanoseconds val) {
                                   const double diff = std::abs(val.count() - mean);
                                   const bool isOutlier = diff > (stdDev * threshold);
                                   if (isOutlier) ++outlierCount;
                                   return isOutlier;
                               }), samples.end());
}

BenchmarkHarness::BenchmarkResult BenchmarkHarness::analyzeResults(const std::string& name,
                                                                 const std::vector<std::chrono::nanoseconds>& samples)
{
    if (samples.empty())
    {
        return BenchmarkResult{name, 0, {}, {}, {}, {}, {}, {}, 0.0, 0, false, "No samples collected"};
    }

    BenchmarkResult result;
    result.name = name;
    result.iterations = samples.size();

    // Sort for percentile calculations
    std::vector<std::chrono::nanoseconds> sortedSamples = samples;
    std::sort(sortedSamples.begin(), sortedSamples.end());

    result.minTime = sortedSamples.front();
    result.maxTime = sortedSamples.back();

    // Calculate mean
    const double sum = std::accumulate(samples.begin(), samples.end(), 0.0,
                                     [](double acc, std::chrono::nanoseconds val) {
                                         return acc + val.count();
                                     });
    const double meanNs = sum / samples.size();
    result.meanTime = std::chrono::nanoseconds(static_cast<long long>(meanNs));

    // Calculate median
    const size_t medianIdx = samples.size() / 2;
    result.medianTime = sortedSamples[medianIdx];

    // Calculate percentiles
    const size_t p95Idx = static_cast<size_t>(samples.size() * 0.95);
    const size_t p99Idx = static_cast<size_t>(samples.size() * 0.99);
    result.p95Time = sortedSamples[std::min(p95Idx, samples.size() - 1)];
    result.p99Time = sortedSamples[std::min(p99Idx, samples.size() - 1)];

    // Calculate standard deviation
    const double variance = std::accumulate(samples.begin(), samples.end(), 0.0,
                                          [meanNs](double acc, std::chrono::nanoseconds val) {
                                              const double diff = val.count() - meanNs;
                                              return acc + diff * diff;
                                          }) / samples.size();
    result.standardDeviationNs = std::sqrt(variance);

    return result;
}

//==============================================================================

RegressionDetector::RegressionDetector()
    : minorThreshold_(0.05f)    // 5% degradation
    , majorThreshold_(0.15f)    // 15% degradation
    , criticalThreshold_(0.30f) // 30% degradation
{}

bool RegressionDetector::loadBaseline(const std::string& baselineFile)
{
    // Implementation would load baseline data from file
    // For now, return false
    return false;
}

bool RegressionDetector::saveBaseline(const std::string& baselineFile,
                                    const std::vector<BenchmarkHarness::BenchmarkResult>& results)
{
    // Implementation would save baseline data to file
    // For now, return false
    return false;
}

std::vector<RegressionDetector::RegressionResult> RegressionDetector::checkRegressions(
    const std::vector<BenchmarkHarness::BenchmarkResult>& results)
{
    std::vector<RegressionResult> regressions;

    for (const auto& result : results)
    {
        auto regression = analyzeBenchmark(result);
        if (regression.regressionDetected)
        {
            regressions.push_back(regression);
        }
    }

    return regressions;
}

void RegressionDetector::setThresholds(float minorThreshold, float majorThreshold, float criticalThreshold)
{
    minorThreshold_ = minorThreshold;
    majorThreshold_ = majorThreshold;
    criticalThreshold_ = criticalThreshold;
}

std::string RegressionDetector::classifyRegression(double degradationPercent) const
{
    if (degradationPercent >= criticalThreshold_)
        return "critical";
    else if (degradationPercent >= majorThreshold_)
        return "major";
    else if (degradationPercent >= minorThreshold_)
        return "minor";
    else
        return "none";
}

RegressionDetector::RegressionResult RegressionDetector::analyzeBenchmark(const BenchmarkHarness::BenchmarkResult& result)
{
    RegressionResult regressionResult;
    regressionResult.benchmarkName = result.name;
    regressionResult.regressionDetected = false;

    // Check against baseline (simplified - would use stored baseline data)
    const auto baselineP95 = std::chrono::nanoseconds(25000); // 25us baseline

    if (result.p95Time > baselineP95)
    {
        regressionResult.regressionDetected = true;
        regressionResult.metric = "P95 latency";
        regressionResult.baselineValue = baselineP95.count();
        regressionResult.currentValue = result.p95Time.count();
        regressionResult.degradationPercent = (result.p95Time.count() - baselineP95.count()) /
                                            static_cast<double>(baselineP95.count()) * 100.0;
        regressionResult.severity = classifyRegression(regressionResult.degradationPercent);
    }

    return regressionResult;
}

} // namespace daw::core::performance
