#include "BenchmarkSystem.h"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <sys/utsname.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/sysinfo.h>
#include <fstream>
#endif

namespace cppmusic::performance {

// Global benchmark system instance
BenchmarkSystem* g_benchmarkSystem = nullptr;

std::vector<double> PerformanceMetrics::getPercentiles(const std::vector<double>& percentiles) const {
    std::vector<uint64_t> samples_copy;

    // Copy samples atomically
    size_t numSamples = std::min(totalCalls.load(std::memory_order_relaxed), SAMPLE_BUFFER_SIZE);
    samples_copy.reserve(numSamples);

    for (size_t i = 0; i < numSamples; ++i) {
        samples_copy.push_back(samples[i].load(std::memory_order_relaxed));
    }

    if (samples_copy.empty()) {
        return std::vector<double>(percentiles.size(), 0.0);
    }

    std::sort(samples_copy.begin(), samples_copy.end());

    std::vector<double> result;
    result.reserve(percentiles.size());

    for (double p : percentiles) {
        if (p <= 0.0) {
            result.push_back(HighResolutionTimer::toMilliseconds(HighResolutionTimer::Duration(samples_copy.front())));
        } else if (p >= 100.0) {
            result.push_back(HighResolutionTimer::toMilliseconds(HighResolutionTimer::Duration(samples_copy.back())));
        } else {
            size_t index = static_cast<size_t>((p / 100.0) * (samples_copy.size() - 1));
            result.push_back(HighResolutionTimer::toMilliseconds(HighResolutionTimer::Duration(samples_copy[index])));
        }
    }

    return result;
}

std::vector<BenchmarkSystem::RegressionResult> BenchmarkSystem::detectRegressions(double thresholdPercent) const {
    std::vector<RegressionResult> regressions;

    std::lock_guard<std::mutex> benchmarkLock(benchmarkMutex_);

    for (const auto& [name, results] : benchmarkResults_) {
        if (results.empty()) continue;

        auto baselineIt = baselineData_.find(name);
        if (baselineIt == baselineData_.end()) continue;

        // Calculate current average from recent results
        double sum = 0.0;
        size_t count = std::min(results.size(), size_t(10)); // Last 10 results
        for (size_t i = results.size() - count; i < results.size(); ++i) {
            sum += results[i].value;
        }
        double currentAverage = sum / count;

        double baselineValue = baselineIt->second;
        double percentageChange = ((currentAverage - baselineValue) / baselineValue) * 100.0;

        RegressionResult result;
        result.testName = name;
        result.currentValue = currentAverage;
        result.baselineValue = baselineValue;
        result.percentageChange = percentageChange;
        result.isRegression = std::abs(percentageChange) > thresholdPercent && percentageChange > 0;

        if (result.isRegression) {
            result.analysis = "Performance regression detected: " +
                            std::to_string(percentageChange) + "% slower than baseline";
        } else if (percentageChange < -thresholdPercent) {
            result.analysis = "Performance improvement: " +
                            std::to_string(-percentageChange) + "% faster than baseline";
        } else {
            result.analysis = "Performance within acceptable range";
        }

        regressions.push_back(result);
    }

    return regressions;
}

BenchmarkSystem::PerformanceReport BenchmarkSystem::generateReport() const {
    PerformanceReport report;
    report.timestamp = getCurrentTimestamp();
    report.systemInfo = collectSystemInfo();

    // Collect metrics data
    std::lock_guard<std::mutex> metricsLock(metricsMutex_);
    for (const auto& [name, metrics] : metrics_) {
        report.averageTimes[name] = metrics->getAverageMs();
        report.peakTimes[name] = metrics->getMaxMs();
        report.percentiles[name] = metrics->getPercentiles({50.0, 90.0, 95.0, 99.0});
    }

    // Memory information
    auto& memTracker = MemoryTracker::getInstance();
    report.totalMemoryUsage = memTracker.getCurrentMemoryUsage();
    report.peakMemoryUsage = memTracker.getPeakMemoryUsage();

    // Regression analysis
    report.regressions = detectRegressions();

    return report;
}

void BenchmarkSystem::exportToJSON(const std::string& filename) const {
    auto report = generateReport();
    std::ofstream file(filename);

    file << "{\n";
    file << "  \"timestamp\": \"" << report.timestamp << "\",\n";

    // System info
    file << "  \"systemInfo\": {\n";
    bool first = true;
    for (const auto& [key, value] : report.systemInfo) {
        if (!first) file << ",\n";
        file << "    \"" << key << "\": \"" << value << "\"";
        first = false;
    }
    file << "\n  },\n";

    // Performance metrics
    file << "  \"metrics\": {\n";
    first = true;
    for (const auto& [name, avgTime] : report.averageTimes) {
        if (!first) file << ",\n";
        file << "    \"" << name << "\": {\n";
        file << "      \"averageMs\": " << avgTime << ",\n";
        file << "      \"peakMs\": " << report.peakTimes.at(name) << ",\n";
        file << "      \"percentiles\": [";

        const auto& percentiles = report.percentiles.at(name);
        for (size_t i = 0; i < percentiles.size(); ++i) {
            if (i > 0) file << ", ";
            file << percentiles[i];
        }
        file << "]\n";
        file << "    }";
        first = false;
    }
    file << "\n  },\n";

    // Memory usage
    file << "  \"memory\": {\n";
    file << "    \"currentUsage\": " << report.totalMemoryUsage << ",\n";
    file << "    \"peakUsage\": " << report.peakMemoryUsage << "\n";
    file << "  },\n";

    // Regressions
    file << "  \"regressions\": [\n";
    first = true;
    for (const auto& regression : report.regressions) {
        if (!first) file << ",\n";
        file << "    {\n";
        file << "      \"testName\": \"" << regression.testName << "\",\n";
        file << "      \"currentValue\": " << regression.currentValue << ",\n";
        file << "      \"baselineValue\": " << regression.baselineValue << ",\n";
        file << "      \"percentageChange\": " << regression.percentageChange << ",\n";
        file << "      \"isRegression\": " << (regression.isRegression ? "true" : "false") << ",\n";
        file << "      \"analysis\": \"" << regression.analysis << "\"\n";
        file << "    }";
        first = false;
    }
    file << "\n  ]\n";
    file << "}\n";
}

void BenchmarkSystem::exportToCSV(const std::string& filename) const {
    auto report = generateReport();
    std::ofstream file(filename);

    // Headers
    file << "Metric,Average(ms),Peak(ms),P50(ms),P90(ms),P95(ms),P99(ms)\n";

    // Data
    for (const auto& [name, avgTime] : report.averageTimes) {
        file << name << "," << avgTime << "," << report.peakTimes.at(name);

        const auto& percentiles = report.percentiles.at(name);
        for (double p : percentiles) {
            file << "," << p;
        }
        file << "\n";
    }
}

void BenchmarkSystem::exportToHTML(const std::string& filename) const {
    auto report = generateReport();
    std::ofstream file(filename);

    file << R"(<!DOCTYPE html>
<html>
<head>
    <title>Performance Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        table { border-collapse: collapse; width: 100%; margin: 20px 0; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        .regression { background-color: #ffebee; }
        .improvement { background-color: #e8f5e8; }
        .chart { width: 100%; height: 400px; margin: 20px 0; }
    </style>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
    <h1>Performance Report</h1>
    <p><strong>Generated:</strong> )" << report.timestamp << R"(</p>

    <h2>System Information</h2>
    <table>)";

    for (const auto& [key, value] : report.systemInfo) {
        file << "<tr><td>" << key << "</td><td>" << value << "</td></tr>";
    }

    file << R"(    </table>

    <h2>Performance Metrics</h2>
    <table>
        <tr>
            <th>Metric</th>
            <th>Average (ms)</th>
            <th>Peak (ms)</th>
            <th>P50 (ms)</th>
            <th>P90 (ms)</th>
            <th>P95 (ms)</th>
            <th>P99 (ms)</th>
        </tr>)";

    for (const auto& [name, avgTime] : report.averageTimes) {
        file << "<tr>";
        file << "<td>" << name << "</td>";
        file << "<td>" << std::fixed << std::setprecision(3) << avgTime << "</td>";
        file << "<td>" << std::fixed << std::setprecision(3) << report.peakTimes.at(name) << "</td>";

        const auto& percentiles = report.percentiles.at(name);
        for (double p : percentiles) {
            file << "<td>" << std::fixed << std::setprecision(3) << p << "</td>";
        }
        file << "</tr>";
    }

    file << R"(    </table>

    <h2>Memory Usage</h2>
    <p><strong>Current:</strong> )" << report.totalMemoryUsage << R"( bytes</p>
    <p><strong>Peak:</strong> )" << report.peakMemoryUsage << R"( bytes</p>

    <h2>Regression Analysis</h2>
    <table>
        <tr>
            <th>Test</th>
            <th>Current (ms)</th>
            <th>Baseline (ms)</th>
            <th>Change (%)</th>
            <th>Status</th>
            <th>Analysis</th>
        </tr>)";

    for (const auto& regression : report.regressions) {
        std::string rowClass = "";
        if (regression.isRegression) {
            rowClass = " class=\"regression\"";
        } else if (regression.percentageChange < -10.0) {
            rowClass = " class=\"improvement\"";
        }

        file << "<tr" << rowClass << ">";
        file << "<td>" << regression.testName << "</td>";
        file << "<td>" << std::fixed << std::setprecision(3) << regression.currentValue << "</td>";
        file << "<td>" << std::fixed << std::setprecision(3) << regression.baselineValue << "</td>";
        file << "<td>" << std::fixed << std::setprecision(2) << regression.percentageChange << "</td>";
        file << "<td>" << (regression.isRegression ? "REGRESSION" : "OK") << "</td>";
        file << "<td>" << regression.analysis << "</td>";
        file << "</tr>";
    }

    file << R"(    </table>

</body>
</html>)";
}

void BenchmarkSystem::loadBaseline(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line, name;
    double value;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        if (std::getline(iss, name, ',') && (iss >> value)) {
            baselineData_[name] = value;
        }
    }
}

void BenchmarkSystem::saveBaseline(const std::string& filename) const {
    std::ofstream file(filename);

    std::lock_guard<std::mutex> metricsLock(metricsMutex_);
    for (const auto& [name, metrics] : metrics_) {
        file << name << "," << metrics->getAverageMs() << "\n";
    }
}

void BenchmarkSystem::runAudioProcessingBenchmarks() {
    // Register all audio processing benchmarks
    registerMetric("AudioBuffer::processBlock");
    registerMetric("SynthVoice::renderNextBlock");
    registerMetric("EffectChain::process");
    registerMetric("MixerChannel::mixSamples");

    // TODO: Implement specific audio processing benchmarks
    // These would test various audio processing scenarios
}

void BenchmarkSystem::runMemoryBenchmarks() {
    auto& memTracker = MemoryTracker::getInstance();

    // Test allocation patterns
    const size_t testSizes[] = {64, 1024, 4096, 65536, 1048576};

    for (size_t size : testSizes) {
        auto start = HighResolutionTimer::now();

        std::vector<void*> allocations;
        for (int i = 0; i < 1000; ++i) {
            void* ptr = std::malloc(size);
            allocations.push_back(ptr);
            memTracker.recordAllocation(ptr, size, "benchmark");
        }

        auto mid = HighResolutionTimer::now();

        for (void* ptr : allocations) {
            memTracker.recordDeallocation(ptr);
            std::free(ptr);
        }

        auto end = HighResolutionTimer::now();

        double allocTime = HighResolutionTimer::toMilliseconds(mid - start);
        double deallocTime = HighResolutionTimer::toMilliseconds(end - mid);

        recordBenchmark("Memory::alloc_" + std::to_string(size), allocTime);
        recordBenchmark("Memory::dealloc_" + std::to_string(size), deallocTime);
    }
}

void BenchmarkSystem::runConcurrencyBenchmarks() {
    // Test lock contention, atomic operations, thread pool performance
    registerMetric("ThreadPool::enqueueTask");
    registerMetric("AtomicQueue::push");
    registerMetric("AtomicQueue::pop");

    // TODO: Implement concurrency benchmarks
}

void BenchmarkSystem::runFullSystemBenchmark() {
    runAudioProcessingBenchmarks();
    runMemoryBenchmarks();
    runConcurrencyBenchmarks();
}

std::string BenchmarkSystem::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::unordered_map<std::string, std::string> BenchmarkSystem::collectSystemInfo() const {
    std::unordered_map<std::string, std::string> info;

    // Get system information
    struct utsname sysInfo;
    if (uname(&sysInfo) == 0) {
        info["OS"] = sysInfo.sysname;
        info["Architecture"] = sysInfo.machine;
        info["Kernel"] = sysInfo.release;
    }

    // CPU information
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    info["CPU"] = line.substr(pos + 2);
                    break;
                }
            }
        }
    }

    // Memory information
#ifdef __linux__
    struct sysinfo memInfo;
    if (sysinfo(&memInfo) == 0) {
        info["TotalRAM"] = std::to_string(memInfo.totalram * memInfo.mem_unit / (1024 * 1024)) + " MB";
        info["FreeRAM"] = std::to_string(memInfo.freeram * memInfo.mem_unit / (1024 * 1024)) + " MB";
    }
#endif

    // Compiler information
    info["Compiler"] =
#ifdef __clang__
        "Clang " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__);
#elif defined(__GNUC__)
        "GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__);
#elif defined(_MSC_VER)
        "MSVC " + std::to_string(_MSC_VER);
#else
        "Unknown";
#endif

    return info;
}

void CPUMonitor::monitorLoop() {
    const auto interval = std::chrono::milliseconds(100);
    std::vector<double> samples;

    while (running_.load()) {
        double usage = calculateCPUUsage();
        currentCPUUsage_.store(usage);

        samples.push_back(usage);
        if (samples.size() > 100) {  // Keep last 100 samples (10 seconds)
            samples.erase(samples.begin());
        }

        // Update average
        double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
        averageCPUUsage_.store(sum / samples.size());

        // Update peak
        double peak = *std::max_element(samples.begin(), samples.end());
        peakCPUUsage_.store(peak);

        std::this_thread::sleep_for(interval);
    }
}

double CPUMonitor::calculateCPUUsage() {
#ifdef __linux__
    static long long lastTotal = 0, lastIdle = 0;

    std::ifstream file("/proc/stat");
    if (!file.is_open()) return 0.0;

    std::string line;
    std::getline(file, line);

    std::istringstream iss(line);
    std::string cpu;
    long long user, nice, system, idle, iowait, irq, softirq, steal;

    iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    long long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long long currentIdle = idle + iowait;

    if (lastTotal != 0) {
        long long totalDiff = total - lastTotal;
        long long idleDiff = currentIdle - lastIdle;

        if (totalDiff > 0) {
            double usage = 100.0 * (totalDiff - idleDiff) / totalDiff;
            lastTotal = total;
            lastIdle = currentIdle;
            return usage;
        }
    }

    lastTotal = total;
    lastIdle = currentIdle;
#endif
    return 0.0;
}

std::vector<PerformanceDashboard::Alert> PerformanceDashboard::checkAlerts() const {
    std::vector<Alert> alerts;
    RealtimeMetrics current = getCurrentMetrics();
    auto now = HighResolutionTimer::now();

    // CPU usage alerts
    if (current.cpuUsage > 80.0) {
        alerts.push_back({
            "HIGH_CPU",
            "CPU usage is " + std::to_string(current.cpuUsage) + "%",
            now,
            current
        });
    }

    // Memory usage alerts
    if (current.memoryUsage > 1073741824) {  // 1GB
        alerts.push_back({
            "HIGH_MEMORY",
            "Memory usage is " + std::to_string(current.memoryUsage / (1024*1024)) + " MB",
            now,
            current
        });
    }

    // Audio dropout alerts
    if (current.audioDropouts > 1.0) {
        alerts.push_back({
            "AUDIO_DROPOUTS",
            "Audio dropouts detected: " + std::to_string(current.audioDropouts) + "%",
            now,
            current
        });
    }

    // Latency alerts
    if (current.audioLatency > 20.0) {  // 20ms
        alerts.push_back({
            "HIGH_LATENCY",
            "Audio latency is " + std::to_string(current.audioLatency) + " ms",
            now,
            current
        });
    }

    return alerts;
}

std::vector<PerformanceTestSuite::TestResult> PerformanceTestSuite::runAllTests() {
    std::vector<TestResult> results;

    for (const auto& test : tests_) {
        TestResult result;
        result.name = test.name;
        result.baselineTime = test.baselineTime;
        result.maxAllowedTime = test.maxAllowedTime;

        try {
            // Setup
            if (test.setup) {
                test.setup();
            }

            // Run benchmark
            auto start = HighResolutionTimer::now();
            result.measuredTime = test.benchmark();
            auto end = HighResolutionTimer::now();

            // Alternative timing if benchmark doesn't return time
            if (result.measuredTime <= 0.0) {
                result.measuredTime = HighResolutionTimer::toMilliseconds(end - start);
            }

            // Evaluate results
            result.passed = result.measuredTime <= result.maxAllowedTime;

            if (result.passed) {
                double improvement = ((result.baselineTime - result.measuredTime) / result.baselineTime) * 100.0;
                if (improvement > 5.0) {
                    result.status = "IMPROVED (" + std::to_string(improvement) + "% faster)";
                } else {
                    result.status = "PASSED";
                }
            } else {
                double regression = ((result.measuredTime - result.maxAllowedTime) / result.maxAllowedTime) * 100.0;
                result.status = "FAILED (" + std::to_string(regression) + "% slower than limit)";
            }

            // Teardown
            if (test.teardown) {
                test.teardown();
            }

        } catch (const std::exception& e) {
            result.measuredTime = -1.0;
            result.passed = false;
            result.status = "ERROR: " + std::string(e.what());
        }

        results.push_back(result);
    }

    return results;
}

} // namespace cppmusic::performance
