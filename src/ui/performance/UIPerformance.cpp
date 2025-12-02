/**
 * @file UIPerformance.cpp
 * @brief Implementation of UI performance monitoring
 */

#include "UIPerformance.hpp"
#include <iomanip>
#include <sstream>

namespace cppmusic::ui::performance {

UIPerformanceTracker& UIPerformanceTracker::getInstance() {
    static UIPerformanceTracker instance;
    return instance;
}

void UIPerformanceTracker::recordTiming(const juce::String& label, double timeMs) {
    if (!enabled_) return;
    
    juce::ScopedLock sl(lock_);
    metrics_[label].update(timeMs);
}

const PerfMetrics* UIPerformanceTracker::getMetrics(const juce::String& label) const {
    juce::ScopedLock sl(lock_);
    auto it = metrics_.find(label);
    return it != metrics_.end() ? &it->second : nullptr;
}

void UIPerformanceTracker::reset() {
    juce::ScopedLock sl(lock_);
    metrics_.clear();
}

void UIPerformanceTracker::printSummary() const {
    juce::ScopedLock sl(lock_);
    
    if (metrics_.empty()) {
        DBG("UI Performance: No metrics recorded");
        return;
    }
    
    DBG("=== UI Performance Summary ===");
    DBG("Label                              Count    Min(ms)  Avg(ms)  Max(ms)");
    DBG("-----------------------------------------------------------------------");
    
    for (const auto& [label, metrics] : metrics_) {
        juce::String line = juce::String(label).paddedRight(' ', 35);
        line += juce::String(metrics.count).paddedLeft(' ', 7);
        line += juce::String(metrics.minMs, 2).paddedLeft(' ', 10);
        line += juce::String(metrics.avgMs, 2).paddedLeft(' ', 10);
        line += juce::String(metrics.maxMs, 2).paddedLeft(' ', 10);
        DBG(line);
    }
    
    DBG("===============================");
}

} // namespace cppmusic::ui::performance
