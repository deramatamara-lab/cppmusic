/**
 * @file UIPerformance.hpp
 * @brief UI performance monitoring and profiling utilities
 *
 * Provides lightweight performance logging for UI operations:
 * - Paint timing for panels
 * - Layout computation timing
 * - Debug performance summaries
 */

#pragma once

#include <juce_core/juce_core.h>
#include <string>
#include <unordered_map>
#include <chrono>

namespace cppmusic::ui::performance {

/**
 * @brief UI performance metrics
 */
struct PerfMetrics {
    double minMs{0.0};
    double maxMs{0.0};
    double avgMs{0.0};
    int count{0};
    
    void update(double timeMs) {
        if (count == 0) {
            minMs = maxMs = avgMs = timeMs;
        } else {
            minMs = juce::jmin(minMs, timeMs);
            maxMs = juce::jmax(maxMs, timeMs);
            avgMs = (avgMs * count + timeMs) / (count + 1);
        }
        ++count;
    }
    
    void reset() {
        minMs = maxMs = avgMs = 0.0;
        count = 0;
    }
};

/**
 * @brief UI performance tracker (singleton)
 */
class UIPerformanceTracker {
public:
    static UIPerformanceTracker& getInstance();
    
    /**
     * @brief Enable/disable performance tracking
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    /**
     * @brief Record a timing measurement
     */
    void recordTiming(const juce::String& label, double timeMs);
    
    /**
     * @brief Get metrics for a label
     */
    const PerfMetrics* getMetrics(const juce::String& label) const;
    
    /**
     * @brief Get all metrics
     */
    const std::unordered_map<juce::String, PerfMetrics>& getAllMetrics() const {
        return metrics_;
    }
    
    /**
     * @brief Reset all metrics
     */
    void reset();
    
    /**
     * @brief Print summary to debug console
     */
    void printSummary() const;
    
private:
    UIPerformanceTracker() = default;
    ~UIPerformanceTracker() = default;
    UIPerformanceTracker(const UIPerformanceTracker&) = delete;
    UIPerformanceTracker& operator=(const UIPerformanceTracker&) = delete;
    
    bool enabled_{false};
    std::unordered_map<juce::String, PerfMetrics> metrics_;
    mutable juce::CriticalSection lock_;
};

/**
 * @brief RAII helper for automatic timing measurements
 * 
 * Usage:
 *   void paint(Graphics& g) override {
 *       UI_PERF_SCOPE("MyComponent::paint");
 *       // ... painting code ...
 *   }
 */
class ScopedPerfTimer {
public:
    explicit ScopedPerfTimer(const juce::String& label)
        : label_(label)
        , start_(std::chrono::high_resolution_clock::now())
    {
    }
    
    ~ScopedPerfTimer() {
        if (UIPerformanceTracker::getInstance().isEnabled()) {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
            UIPerformanceTracker::getInstance().recordTiming(label_, duration / 1000.0);
        }
    }
    
private:
    juce::String label_;
    std::chrono::high_resolution_clock::time_point start_;
};

} // namespace cppmusic::ui::performance

// Convenience macros for performance logging
#ifdef NDEBUG
    #define UI_PERF_SCOPE(label) ((void)0)
    #define UI_PERF_LOG(label, code) code
#else
    #define UI_PERF_SCOPE(label) cppmusic::ui::performance::ScopedPerfTimer _perfTimer(label)
    #define UI_PERF_LOG(label, code) \
        do { \
            cppmusic::ui::performance::ScopedPerfTimer _perfTimer(label); \
            code; \
        } while (0)
#endif
