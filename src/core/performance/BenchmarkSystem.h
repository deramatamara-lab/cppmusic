#pragma once

#include <chrono>
#include <atomic>
#include <array>
#include <unordered_map>
#include <string>
#include <memory>
#include <thread>
#include <functional>
#include <vector>
#include <mutex>
#include <fstream>

namespace cppmusic::performance {

/**
 * High-precision performance measurement utilities
 *
 * Features:
 * - CPU profiling with call stack tracking
 * - Memory allocation tracking
 * - Real-time performance metrics
 * - Automated regression detection
 * - Multi-threaded performance analysis
 * - Integration with CI/CD pipelines
 */

class HighResolutionTimer {
public:
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Duration = std::chrono::nanoseconds;

    static TimePoint now() {
        return std::chrono::high_resolution_clock::now();
    }

    static double toMilliseconds(Duration duration) {
        return std::chrono::duration<double, std::milli>(duration).count();
    }

    static double toMicroseconds(Duration duration) {
        return std::chrono::duration<double, std::micro>(duration).count();
    }
};

struct PerformanceMetrics {
    std::atomic<uint64_t> totalCalls{0};
    std::atomic<uint64_t> totalTime{0};  // nanoseconds
    std::atomic<uint64_t> minTime{UINT64_MAX};
    std::atomic<uint64_t> maxTime{0};
    std::atomic<uint64_t> memoryAllocated{0};
    std::atomic<uint64_t> memoryPeak{0};
    std::atomic<double> cpuUsage{0.0};

    // Percentile tracking (lockless ring buffer)
    static constexpr size_t SAMPLE_BUFFER_SIZE = 1024;
    std::array<std::atomic<uint64_t>, SAMPLE_BUFFER_SIZE> samples{};
    std::atomic<size_t> sampleIndex{0};

    void recordSample(uint64_t nanoseconds) {
        totalCalls.fetch_add(1, std::memory_order_relaxed);
        totalTime.fetch_add(nanoseconds, std::memory_order_relaxed);

        // Update min/max atomically
        uint64_t currentMin = minTime.load(std::memory_order_relaxed);
        while (nanoseconds < currentMin &&
               !minTime.compare_exchange_weak(currentMin, nanoseconds, std::memory_order_relaxed)) {}

        uint64_t currentMax = maxTime.load(std::memory_order_relaxed);
        while (nanoseconds > currentMax &&
               !maxTime.compare_exchange_weak(currentMax, nanoseconds, std::memory_order_relaxed)) {}

        // Store sample for percentile calculation
        size_t index = sampleIndex.fetch_add(1, std::memory_order_relaxed) % SAMPLE_BUFFER_SIZE;
        samples[index].store(nanoseconds, std::memory_order_relaxed);
    }

    double getAverageMs() const {
        uint64_t calls = totalCalls.load(std::memory_order_relaxed);
        if (calls == 0) return 0.0;
        return HighResolutionTimer::toMilliseconds(
            HighResolutionTimer::Duration(totalTime.load(std::memory_order_relaxed))) / calls;
    }

    double getMinMs() const {
        uint64_t min = minTime.load(std::memory_order_relaxed);
        return min == UINT64_MAX ? 0.0 : HighResolutionTimer::toMilliseconds(HighResolutionTimer::Duration(min));
    }

    double getMaxMs() const {
        return HighResolutionTimer::toMilliseconds(HighResolutionTimer::Duration(maxTime.load(std::memory_order_relaxed)));
    }

    std::vector<double> getPercentiles(const std::vector<double>& percentiles) const;
};

class ScopedProfiler {
public:
    ScopedProfiler(const std::string& name, PerformanceMetrics& metrics)
        : name_(name), metrics_(metrics), startTime_(HighResolutionTimer::now()) {}

    ~ScopedProfiler() {
        auto endTime = HighResolutionTimer::now();
        auto duration = std::chrono::duration_cast<HighResolutionTimer::Duration>(endTime - startTime_);
        metrics_.recordSample(duration.count());
    }

private:
    std::string name_;
    PerformanceMetrics& metrics_;
    HighResolutionTimer::TimePoint startTime_;
};

// Macro for easy profiling
#define PROFILE_SCOPE(name, metrics) \
    cppmusic::performance::ScopedProfiler _prof(name, metrics)

class MemoryTracker {
public:
    struct AllocationInfo {
        size_t size;
        std::chrono::high_resolution_clock::time_point timestamp;
        std::thread::id threadId;
        std::string context;
    };

    static MemoryTracker& getInstance() {
        static MemoryTracker instance;
        return instance;
    }

    void recordAllocation(void* ptr, size_t size, const std::string& context = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_[ptr] = {size, HighResolutionTimer::now(), std::this_thread::get_id(), context};
        totalAllocated_ += size;
        peakMemory_ = std::max(peakMemory_, getCurrentMemoryUsage());
    }

    void recordDeallocation(void* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = allocations_.find(ptr);
        if (it != allocations_.end()) {
            totalAllocated_ -= it->second.size;
            allocations_.erase(it);
        }
    }

    size_t getCurrentMemoryUsage() const {
        size_t total = 0;
        for (const auto& [ptr, info] : allocations_) {
            total += info.size;
        }
        return total;
    }

    size_t getPeakMemoryUsage() const { return peakMemory_; }
    size_t getTotalAllocated() const { return totalAllocated_; }

    std::vector<AllocationInfo> getActiveAllocations() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<AllocationInfo> result;
        for (const auto& [ptr, info] : allocations_) {
            result.push_back(info);
        }
        return result;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<void*, AllocationInfo> allocations_;
    std::atomic<size_t> totalAllocated_{0};
    std::atomic<size_t> peakMemory_{0};
};

class BenchmarkSystem {
public:
    BenchmarkSystem() = default;
    ~BenchmarkSystem() = default;

    // Register a performance metric
    void registerMetric(const std::string& name) {
        std::lock_guard<std::mutex> lock(metricsMutex_);
        if (metrics_.find(name) == metrics_.end()) {
            metrics_[name] = std::make_unique<PerformanceMetrics>();
        }
    }

    // Get metrics for profiling
    PerformanceMetrics* getMetrics(const std::string& name) {
        std::lock_guard<std::mutex> lock(metricsMutex_);
        auto it = metrics_.find(name);
        return it != metrics_.end() ? it->second.get() : nullptr;
    }

    // Record a benchmark result
    void recordBenchmark(const std::string& name, double value, const std::string& unit = "ms") {
        std::lock_guard<std::mutex> lock(benchmarkMutex_);
        benchmarkResults_[name].push_back({value, unit, HighResolutionTimer::now()});
    }

    // Run automated performance regression tests
    struct RegressionResult {
        std::string testName;
        double currentValue;
        double baselineValue;
        double percentageChange;
        bool isRegression;
        std::string analysis;
    };

    std::vector<RegressionResult> detectRegressions(double thresholdPercent = 10.0) const;

    // Generate performance report
    struct PerformanceReport {
        std::string timestamp;
        std::unordered_map<std::string, std::string> systemInfo;
        std::unordered_map<std::string, double> averageTimes;
        std::unordered_map<std::string, double> peakTimes;
        std::unordered_map<std::string, std::vector<double>> percentiles;
        size_t totalMemoryUsage;
        size_t peakMemoryUsage;
        std::vector<RegressionResult> regressions;
    };

    PerformanceReport generateReport() const;

    // Export results to various formats
    void exportToJSON(const std::string& filename) const;
    void exportToCSV(const std::string& filename) const;
    void exportToHTML(const std::string& filename) const;

    // Load baseline data for regression testing
    void loadBaseline(const std::string& filename);
    void saveBaseline(const std::string& filename) const;

    // Automated benchmark runners
    void runAudioProcessingBenchmarks();
    void runMemoryBenchmarks();
    void runConcurrencyBenchmarks();
    void runFullSystemBenchmark();

private:
    struct BenchmarkEntry {
        double value;
        std::string unit;
        HighResolutionTimer::TimePoint timestamp;
    };

    mutable std::mutex metricsMutex_;
    mutable std::mutex benchmarkMutex_;

    std::unordered_map<std::string, std::unique_ptr<PerformanceMetrics>> metrics_;
    std::unordered_map<std::string, std::vector<BenchmarkEntry>> benchmarkResults_;
    std::unordered_map<std::string, double> baselineData_;

    std::string getCurrentTimestamp() const;
    std::unordered_map<std::string, std::string> collectSystemInfo() const;
};

// CPU Usage Monitor
class CPUMonitor {
public:
    CPUMonitor() : running_(false) {}

    void start() {
        if (!running_.exchange(true)) {
            monitorThread_ = std::thread(&CPUMonitor::monitorLoop, this);
        }
    }

    void stop() {
        if (running_.exchange(false)) {
            if (monitorThread_.joinable()) {
                monitorThread_.join();
            }
        }
    }

    double getCurrentCPUUsage() const { return currentCPUUsage_.load(); }
    double getAverageCPUUsage() const { return averageCPUUsage_.load(); }
    double getPeakCPUUsage() const { return peakCPUUsage_.load(); }

private:
    std::atomic<bool> running_;
    std::thread monitorThread_;
    std::atomic<double> currentCPUUsage_{0.0};
    std::atomic<double> averageCPUUsage_{0.0};
    std::atomic<double> peakCPUUsage_{0.0};

    void monitorLoop();
    double calculateCPUUsage();
};

// Real-time performance dashboard
class PerformanceDashboard {
public:
    struct RealtimeMetrics {
        double audioLatency;        // ms
        double cpuUsage;           // %
        size_t memoryUsage;        // bytes
        double audioDropouts;      // %
        int activeVoices;
        double processingLoad;     // %
    };

    void updateMetrics(const RealtimeMetrics& metrics) {
        currentMetrics_.store(metrics);
        metricsHistory_.push_back({metrics, HighResolutionTimer::now()});

        // Keep only recent history (last 1000 entries)
        if (metricsHistory_.size() > 1000) {
            metricsHistory_.erase(metricsHistory_.begin());
        }
    }

    RealtimeMetrics getCurrentMetrics() const {
        return currentMetrics_.load();
    }

    std::vector<std::pair<RealtimeMetrics, HighResolutionTimer::TimePoint>> getHistory() const {
        return metricsHistory_;
    }

    // Alert system for performance issues
    struct Alert {
        std::string type;
        std::string message;
        HighResolutionTimer::TimePoint timestamp;
        RealtimeMetrics metrics;
    };

    std::vector<Alert> checkAlerts() const;

private:
    std::atomic<RealtimeMetrics> currentMetrics_{};
    std::vector<std::pair<RealtimeMetrics, HighResolutionTimer::TimePoint>> metricsHistory_;
    mutable std::mutex historyMutex_;
};

// Global benchmark system instance
extern BenchmarkSystem* g_benchmarkSystem;

// Convenience functions
inline void initializeBenchmarkSystem() {
    if (!g_benchmarkSystem) {
        g_benchmarkSystem = new BenchmarkSystem();
    }
}

inline void shutdownBenchmarkSystem() {
    delete g_benchmarkSystem;
    g_benchmarkSystem = nullptr;
}

inline PerformanceMetrics* getMetrics(const std::string& name) {
    if (g_benchmarkSystem) {
        g_benchmarkSystem->registerMetric(name);
        return g_benchmarkSystem->getMetrics(name);
    }
    return nullptr;
}

// Automated testing framework integration
class PerformanceTestSuite {
public:
    struct TestCase {
        std::string name;
        std::function<void()> setup;
        std::function<double()> benchmark;  // Returns time in ms
        std::function<void()> teardown;
        double baselineTime;  // Expected baseline in ms
        double maxAllowedTime;  // Maximum allowed time in ms
    };

    void addTest(const TestCase& test) {
        tests_.push_back(test);
    }

    struct TestResult {
        std::string name;
        double measuredTime;
        double baselineTime;
        double maxAllowedTime;
        bool passed;
        std::string status;
    };

    std::vector<TestResult> runAllTests();

private:
    std::vector<TestCase> tests_;
};

} // namespace cppmusic::performance
