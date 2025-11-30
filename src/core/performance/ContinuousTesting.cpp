#include "AudioBenchmarks.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace cppmusic::performance {

ContinuousPerformanceTesting::ContinuousPerformanceTesting(const TestConfiguration& config)
    : config_(config) {
    startTime_ = std::chrono::high_resolution_clock::now();
}

ContinuousPerformanceTesting::CITestResults ContinuousPerformanceTesting::runCITestSuite() {
    CITestResults results;
    results.allTestsPassed = true;

    // Run all test categories
    bool quickPassed = runQuickPerformanceTests();
    bool memoryPassed = runMemoryLeakTests();
    bool stabilityPassed = runStabilityTests();
    bool regressionPassed = runRegressionTests();

    results.allTestsPassed = quickPassed && memoryPassed && stabilityPassed && regressionPassed;

    if (!quickPassed) results.failedTests.push_back("QuickPerformanceTests");
    if (!memoryPassed) results.failedTests.push_back("MemoryLeakTests");
    if (!stabilityPassed) results.failedTests.push_back("StabilityTests");
    if (!regressionPassed) results.failedTests.push_back("RegressionTests");

    auto endTime = std::chrono::high_resolution_clock::now();
    results.totalExecutionTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime_).count();

    // Generate detailed report
    std::ostringstream report;
    report << "Performance Test Suite Results\n";
    report << "==============================\n\n";
    report << "Execution Time: " << std::fixed << std::setprecision(2) << results.totalExecutionTimeMs << " ms\n";
    report << "Tests Passed: " << (results.allTestsPassed ? "YES" : "NO") << "\n\n";

    if (!results.failedTests.empty()) {
        report << "Failed Tests:\n";
        for (const auto& test : results.failedTests) {
            report << "  - " << test << "\n";
        }
        report << "\n";
    }

    if (!results.warnings.empty()) {
        report << "Warnings:\n";
        for (const auto& warning : results.warnings) {
            report << "  - " << warning << "\n";
        }
        report << "\n";
    }

    results.detailedReport = report.str();

    return results;
}

bool ContinuousPerformanceTesting::runQuickPerformanceTests() {
    bool allPassed = true;

    // Initialize benchmarking system
    initializeAudioBenchmarking();

    // Quick EQ processing test
    auto eqResults = g_audioBenchmarks->benchmarkAnalogEQ(48000.0, 512, 2, false, 5);

    if (eqResults.processingTimeMs > config_.maxAllowedLatency) {
        testResults_.push_back("FAIL: EQ processing time " +
                              std::to_string(eqResults.processingTimeMs) +
                              "ms exceeds limit " +
                              std::to_string(config_.maxAllowedLatency) + "ms");
        allPassed = false;
    } else {
        testResults_.push_back("PASS: EQ processing time " +
                              std::to_string(eqResults.processingTimeMs) + "ms");
    }

    if (eqResults.cpuUsagePercent > config_.maxAllowedCPU) {
        testResults_.push_back("FAIL: EQ CPU usage " +
                              std::to_string(eqResults.cpuUsagePercent) +
                              "% exceeds limit " +
                              std::to_string(config_.maxAllowedCPU) + "%");
        allPassed = false;
    } else {
        testResults_.push_back("PASS: EQ CPU usage " +
                              std::to_string(eqResults.cpuUsagePercent) + "%");
    }

    if (!eqResults.realtimeSafe) {
        testResults_.push_back("FAIL: EQ is not realtime safe");
        allPassed = false;
    } else {
        testResults_.push_back("PASS: EQ is realtime safe");
    }

    // Quick latency test
    auto latencyResults = g_audioBenchmarks->measureSystemLatency();

    if (latencyResults.inputToOutputMs > config_.maxAllowedLatency) {
        testResults_.push_back("FAIL: System latency " +
                              std::to_string(latencyResults.inputToOutputMs) +
                              "ms exceeds limit");
        allPassed = false;
    } else {
        testResults_.push_back("PASS: System latency " +
                              std::to_string(latencyResults.inputToOutputMs) + "ms");
    }

    return allPassed;
}

bool ContinuousPerformanceTesting::runMemoryLeakTests() {
    bool allPassed = true;

    auto& memTracker = MemoryTracker::getInstance();
    size_t initialMemory = memTracker.getCurrentMemoryUsage();

    // Perform memory-intensive operations
    {
        std::vector<void*> allocations;

        // Allocate memory
        for (int i = 0; i < 1000; ++i) {
            void* ptr = std::malloc(1024);
            allocations.push_back(ptr);
            memTracker.recordAllocation(ptr, 1024, "leak_test");
        }

        // Check peak memory usage
        size_t peakMemory = memTracker.getCurrentMemoryUsage();
        if (peakMemory > config_.maxAllowedMemory) {
            testResults_.push_back("FAIL: Peak memory usage " +
                                  std::to_string(peakMemory) +
                                  " exceeds limit " +
                                  std::to_string(config_.maxAllowedMemory));
            allPassed = false;
        }

        // Clean up
        for (void* ptr : allocations) {
            memTracker.recordDeallocation(ptr);
            std::free(ptr);
        }
    }

    // Check for memory leaks
    size_t finalMemory = memTracker.getCurrentMemoryUsage();
    if (finalMemory > initialMemory) {
        size_t leaked = finalMemory - initialMemory;
        testResults_.push_back("FAIL: Memory leak detected: " + std::to_string(leaked) + " bytes");
        allPassed = false;
    } else {
        testResults_.push_back("PASS: No memory leaks detected");
    }

    return allPassed;
}

bool ContinuousPerformanceTesting::runStabilityTests() {
    bool allPassed = true;

    // Run extended processing to test stability
    const int numIterations = config_.enableLongRunningTests ? 10000 : 1000;

    for (int i = 0; i < numIterations; ++i) {
        auto eqResults = g_audioBenchmarks->benchmarkAnalogEQ(48000.0, 512, 2, false, 5);

        // Check for numerical stability
        if (eqResults.filterStability < 0.99) {
            testResults_.push_back("FAIL: Filter instability detected at iteration " + std::to_string(i));
            allPassed = false;
            break;
        }

        // Check for performance degradation over time
        if (i > 100 && eqResults.processingTimeMs > config_.maxAllowedLatency * 1.5) {
            testResults_.push_back("FAIL: Performance degradation detected at iteration " + std::to_string(i));
            allPassed = false;
            break;
        }
    }

    if (allPassed) {
        testResults_.push_back("PASS: Stability test completed " + std::to_string(numIterations) + " iterations");
    }

    return allPassed;
}

bool ContinuousPerformanceTesting::runRegressionTests() {
    bool allPassed = true;

    // Load baseline data if available
    if (g_benchmarkSystem) {
        g_benchmarkSystem->loadBaseline("performance_baseline.csv");

        // Run current benchmarks
        g_audioBenchmarks->runFullAudioBenchmark();

        // Check for regressions
        auto regressions = g_benchmarkSystem->detectRegressions(config_.regressionThreshold);

        for (const auto& regression : regressions) {
            if (regression.isRegression) {
                testResults_.push_back("FAIL: Regression detected in " + regression.testName +
                                      ": " + std::to_string(regression.percentageChange) + "% slower");
                allPassed = false;
            } else if (regression.percentageChange < -config_.regressionThreshold) {
                testResults_.push_back("INFO: Improvement detected in " + regression.testName +
                                      ": " + std::to_string(-regression.percentageChange) + "% faster");
            }
        }

        if (allPassed && regressions.empty()) {
            testResults_.push_back("PASS: No performance regressions detected");
        }
    } else {
        testResults_.push_back("WARN: Benchmark system not available for regression testing");
    }

    return allPassed;
}

void ContinuousPerformanceTesting::generateJUnitReport(const std::string& filename) const {
    std::ofstream file(filename);

    file << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n";
    file << R"(<testsuite name="PerformanceTests" tests=")" << testResults_.size() << R"(" failures="0" errors="0">)" << "\n";

    for (size_t i = 0; i < testResults_.size(); ++i) {
        const std::string& result = testResults_[i];
        std::string status = result.substr(0, 4);
        std::string testName = "Test_" + std::to_string(i + 1);
        std::string message = result.substr(6); // Skip "PASS: " or "FAIL: "

        file << R"(  <testcase name=")" << testName << R"(" classname="PerformanceTests">)" << "\n";

        if (status == "FAIL") {
            file << R"(    <failure message=")" << message << R"("/>)" << "\n";
        } else if (status == "WARN") {
            file << R"(    <system-out>)" << message << R"(</system-out>)" << "\n";
        }

        file << R"(  </testcase>)" << "\n";
    }

    file << R"(</testsuite>)" << "\n";
}

void ContinuousPerformanceTesting::generateMarkdownReport(const std::string& filename) const {
    std::ofstream file(filename);

    auto endTime = std::chrono::high_resolution_clock::now();
    double totalTime = std::chrono::duration<double, std::milli>(endTime - startTime_).count();

    file << "# Performance Test Report\n\n";
    file << "**Generated:** " << std::put_time(std::localtime(&std::time(nullptr)), "%Y-%m-%d %H:%M:%S") << "\n";
    file << "**Duration:** " << std::fixed << std::setprecision(2) << totalTime << " ms\n\n";

    // Summary
    int passed = 0, failed = 0, warnings = 0;
    for (const std::string& result : testResults_) {
        if (result.substr(0, 4) == "PASS") passed++;
        else if (result.substr(0, 4) == "FAIL") failed++;
        else if (result.substr(0, 4) == "WARN") warnings++;
    }

    file << "## Summary\n\n";
    file << "| Status | Count |\n";
    file << "|--------|-------|\n";
    file << "| ✅ Passed | " << passed << " |\n";
    file << "| ❌ Failed | " << failed << " |\n";
    file << "| ⚠️ Warnings | " << warnings << " |\n";
    file << "| **Total** | " << testResults_.size() << " |\n\n";

    // Detailed results
    file << "## Detailed Results\n\n";

    for (const std::string& result : testResults_) {
        std::string emoji = "❓";
        if (result.substr(0, 4) == "PASS") emoji = "✅";
        else if (result.substr(0, 4) == "FAIL") emoji = "❌";
        else if (result.substr(0, 4) == "WARN") emoji = "⚠️";
        else if (result.substr(0, 4) == "INFO") emoji = "ℹ️";

        file << emoji << " " << result.substr(6) << "\n";
    }

    file << "\n## Configuration\n\n";
    file << "- **Max Allowed Latency:** " << config_.maxAllowedLatency << " ms\n";
    file << "- **Max Allowed CPU:** " << config_.maxAllowedCPU << " %\n";
    file << "- **Max Allowed Memory:** " << config_.maxAllowedMemory / (1024*1024) << " MB\n";
    file << "- **Regression Threshold:** " << config_.regressionThreshold << " %\n";
    file << "- **Stress Tests Enabled:** " << (config_.enableStressTests ? "Yes" : "No") << "\n";
    file << "- **Long Running Tests:** " << (config_.enableLongRunningTests ? "Yes" : "No") << "\n";
}

void ContinuousPerformanceTesting::generateMetricsFile(const std::string& filename) const {
    std::ofstream file(filename);

    // Generate metrics in a format suitable for monitoring systems
    file << "# Performance metrics for monitoring\n";

    // Extract numeric values from test results
    for (const std::string& result : testResults_) {
        if (result.find("processing time") != std::string::npos) {
            // Extract processing time value
            size_t pos = result.find(" time ");
            if (pos != std::string::npos) {
                std::string timeStr = result.substr(pos + 5);
                size_t msPos = timeStr.find("ms");
                if (msPos != std::string::npos) {
                    std::string value = timeStr.substr(0, msPos);
                    file << "eq_processing_time_ms " << value << "\n";
                }
            }
        }
        else if (result.find("CPU usage") != std::string::npos) {
            // Extract CPU usage value
            size_t pos = result.find("usage ");
            if (pos != std::string::npos) {
                std::string cpuStr = result.substr(pos + 6);
                size_t pctPos = cpuStr.find("%");
                if (pctPos != std::string::npos) {
                    std::string value = cpuStr.substr(0, pctPos);
                    file << "eq_cpu_usage_percent " << value << "\n";
                }
            }
        }
        else if (result.find("latency") != std::string::npos) {
            // Extract latency value
            size_t pos = result.find("latency ");
            if (pos != std::string::npos) {
                std::string latStr = result.substr(pos + 8);
                size_t msPos = latStr.find("ms");
                if (msPos != std::string::npos) {
                    std::string value = latStr.substr(0, msPos);
                    file << "system_latency_ms " << value << "\n";
                }
            }
        }
    }

    // Test execution time
    auto endTime = std::chrono::high_resolution_clock::now();
    double totalTime = std::chrono::duration<double, std::milli>(endTime - startTime_).count();
    file << "test_execution_time_ms " << std::fixed << std::setprecision(2) << totalTime << "\n";

    // Test counts
    int passed = 0, failed = 0;
    for (const std::string& result : testResults_) {
        if (result.substr(0, 4) == "PASS") passed++;
        else if (result.substr(0, 4) == "FAIL") failed++;
    }

    file << "tests_passed " << passed << "\n";
    file << "tests_failed " << failed << "\n";
    file << "tests_total " << testResults_.size() << "\n";
}

std::vector<PerformanceOptimizer::OptimizationSuggestion> PerformanceOptimizer::analyzePerformance(
    const BenchmarkSystem::PerformanceReport& report) {

    std::vector<OptimizationSuggestion> suggestions;

    // Analyze CPU usage
    auto cpuSuggestions = analyzeCPUUsage(report.averageTimes);
    suggestions.insert(suggestions.end(), cpuSuggestions.begin(), cpuSuggestions.end());

    // Analyze memory usage
    auto memorySuggestions = analyzeMemoryUsage(report.totalMemoryUsage, report.peakMemoryUsage);
    suggestions.insert(suggestions.end(), memorySuggestions.begin(), memorySuggestions.end());

    // Analyze latency
    auto latencySuggestions = analyzeLatency(report.averageTimes);
    suggestions.insert(suggestions.end(), latencySuggestions.begin(), latencySuggestions.end());

    return suggestions;
}

std::vector<PerformanceOptimizer::OptimizationSuggestion> PerformanceOptimizer::analyzeCPUUsage(
    const std::unordered_map<std::string, double>& metrics) {

    std::vector<OptimizationSuggestion> suggestions;

    for (const auto& [name, time] : metrics) {
        if (time > 5.0) { // > 5ms processing time
            OptimizationSuggestion suggestion;
            suggestion.category = "CPU";
            suggestion.priority = 4;
            suggestion.autoApplicable = false;

            if (name.find("EQ") != std::string::npos) {
                suggestion.suggestion = "Consider enabling SIMD optimizations for EQ processing";
                suggestion.potentialImprovement = 20.0;
            } else if (name.find("Synth") != std::string::npos) {
                suggestion.suggestion = "Optimize synthesizer voice allocation and rendering";
                suggestion.potentialImprovement = 15.0;
            } else {
                suggestion.suggestion = "Profile " + name + " for optimization opportunities";
                suggestion.potentialImprovement = 10.0;
            }

            suggestions.push_back(suggestion);
        }
    }

    return suggestions;
}

std::vector<PerformanceOptimizer::OptimizationSuggestion> PerformanceOptimizer::analyzeMemoryUsage(
    size_t current, size_t peak) {

    std::vector<OptimizationSuggestion> suggestions;

    if (peak > 100 * 1024 * 1024) { // > 100MB peak
        OptimizationSuggestion suggestion;
        suggestion.category = "Memory";
        suggestion.suggestion = "Consider implementing memory pooling to reduce peak usage";
        suggestion.potentialImprovement = 30.0;
        suggestion.priority = 3;
        suggestion.autoApplicable = false;
        suggestions.push_back(suggestion);
    }

    if (current > 50 * 1024 * 1024) { // > 50MB sustained
        OptimizationSuggestion suggestion;
        suggestion.category = "Memory";
        suggestion.suggestion = "Review memory allocations for potential leaks or excessive usage";
        suggestion.potentialImprovement = 25.0;
        suggestion.priority = 4;
        suggestion.autoApplicable = false;
        suggestions.push_back(suggestion);
    }

    return suggestions;
}

std::vector<PerformanceOptimizer::OptimizationSuggestion> PerformanceOptimizer::analyzeLatency(
    const std::unordered_map<std::string, double>& latencies) {

    std::vector<OptimizationSuggestion> suggestions;

    for (const auto& [name, latency] : latencies) {
        if (latency > 10.0) { // > 10ms latency
            OptimizationSuggestion suggestion;
            suggestion.category = "Latency";
            suggestion.suggestion = "Optimize " + name + " for lower latency processing";
            suggestion.potentialImprovement = 50.0;
            suggestion.priority = 5;
            suggestion.autoApplicable = false;
            suggestions.push_back(suggestion);
        }
    }

    return suggestions;
}

} // namespace cppmusic::performance
