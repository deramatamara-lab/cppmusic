#pragma once

#include <atomic>
#include <chrono>
#include <array>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace daw::core::utilities
{

/**
 * @brief Real-time performance monitor
 * 
 * Tracks CPU load, xrun detection, and latency metrics.
 * Provides percentile tracking (P50, P95, P99) for performance analysis.
 * Follows DAW_DEV_RULES: lock-free, real-time safe.
 */
class PerformanceMonitor
{
public:
    PerformanceMonitor() noexcept
        : cpuLoad(0.0f)
        , xrunCount(0)
        , latencySamples(0)
        , lastProcessTime()
        , accumulatedProcessTime(0)
        , processBlockCount(0)
        , historyIndex(0)
    {
        std::fill(processTimeHistory.begin(), processTimeHistory.end(), std::chrono::nanoseconds(0));
    }

    ~PerformanceMonitor() = default;

    // Non-copyable, non-movable (atomics prevent moves)
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

    /**
     * @brief Record a process block execution time
     * @param processTime Time taken to process the block
     * @param numSamples Number of samples processed
     * @param sampleRate Current sample rate
     */
    void recordProcessTime(std::chrono::nanoseconds processTime, int numSamples, double sampleRate) noexcept
    {
        // Store in history for percentile calculation
        const auto idx = historyIndex.load(std::memory_order_relaxed);
        processTimeHistory[idx] = processTime;
        historyIndex.store((idx + 1) % historySize, std::memory_order_release);
        
        // Update running average
        accumulatedProcessTime += processTime;
        processBlockCount.fetch_add(1, std::memory_order_relaxed);
        
        // Calculate CPU load (process time / available time)
        const auto availableTime = std::chrono::nanoseconds(
            static_cast<int64_t>((numSamples / sampleRate) * 1e9));
        
        if (availableTime.count() > 0)
        {
            const auto load = static_cast<float>(processTime.count()) / static_cast<float>(availableTime.count());
            cpuLoad.store(load, std::memory_order_release);
        }
        
        // Check for xrun (process time > available time)
        if (processTime > availableTime)
        {
            xrunCount.fetch_add(1, std::memory_order_relaxed);
        }
    }

    /**
     * @brief Get current CPU load (0.0 to 1.0+)
     */
    [[nodiscard]] float getCpuLoad() const noexcept
    {
        return cpuLoad.load(std::memory_order_acquire);
    }

    /**
     * @brief Get CPU load as percentage
     */
    [[nodiscard]] float getCpuLoadPercent() const noexcept
    {
        return getCpuLoad() * 100.0f;
    }

    /**
     * @brief Get xrun count (total since last reset)
     */
    [[nodiscard]] uint64_t getXrunCount() const noexcept
    {
        return xrunCount.load(std::memory_order_acquire);
    }

    /**
     * @brief Reset xrun counter
     */
    void resetXrunCount() noexcept
    {
        xrunCount.store(0, std::memory_order_release);
    }

    /**
     * @brief Calculate percentile from process time history
     * @param percentile Percentile (0.0 to 1.0, e.g. 0.95 for P95)
     * @return Process time at percentile in nanoseconds
     */
    [[nodiscard]] std::chrono::nanoseconds getPercentileProcessTime(float percentile) const noexcept
    {
        const auto count = processBlockCount.load(std::memory_order_acquire);
        if (count == 0)
            return std::chrono::nanoseconds(0);
        
        // Copy history for sorting (const operation on copy)
        auto sorted = processTimeHistory;
        const auto actualCount = std::min(count, static_cast<uint64_t>(historySize));
        std::sort(sorted.begin(), sorted.begin() + static_cast<ptrdiff_t>(actualCount));
        
        const auto index = static_cast<size_t>(std::floor(percentile * actualCount));
        if (index >= actualCount)
            return sorted[actualCount - 1];
        
        return sorted[index];
    }

    /**
     * @brief Get P50 (median) process time
     */
    [[nodiscard]] std::chrono::nanoseconds getP50ProcessTime() const noexcept
    {
        return getPercentileProcessTime(0.50f);
    }

    /**
     * @brief Get P95 process time
     */
    [[nodiscard]] std::chrono::nanoseconds getP95ProcessTime() const noexcept
    {
        return getPercentileProcessTime(0.95f);
    }

    /**
     * @brief Get P99 process time
     */
    [[nodiscard]] std::chrono::nanoseconds getP99ProcessTime() const noexcept
    {
        return getPercentileProcessTime(0.99f);
    }

    /**
     * @brief Set latency in samples
     */
    void setLatencySamples(int samples) noexcept
    {
        latencySamples.store(samples, std::memory_order_release);
    }

    /**
     * @brief Get latency in samples
     */
    [[nodiscard]] int getLatencySamples() const noexcept
    {
        return latencySamples.load(std::memory_order_acquire);
    }

    /**
     * @brief Get latency in milliseconds (requires sample rate)
     */
    [[nodiscard]] float getLatencyMs(double sampleRate) const noexcept
    {
        const auto samples = latencySamples.load(std::memory_order_acquire);
        return static_cast<float>(samples / sampleRate * 1000.0);
    }

    /**
     * @brief Reset all statistics
     */
    void reset() noexcept
    {
        cpuLoad.store(0.0f, std::memory_order_release);
        xrunCount.store(0, std::memory_order_release);
        accumulatedProcessTime = std::chrono::nanoseconds(0);
        processBlockCount.store(0, std::memory_order_release);
        std::fill(processTimeHistory.begin(), processTimeHistory.end(), std::chrono::nanoseconds(0));
        historyIndex.store(0, std::memory_order_release);
    }

private:
    static constexpr size_t historySize = 1024; // Store last 1024 process times
    
    std::atomic<float> cpuLoad;
    std::atomic<uint64_t> xrunCount;
    std::atomic<int> latencySamples;
    
    std::chrono::high_resolution_clock::time_point lastProcessTime;
    std::chrono::nanoseconds accumulatedProcessTime;
    mutable std::atomic<uint64_t> processBlockCount;
    
    alignas(64) mutable std::array<std::chrono::nanoseconds, historySize> processTimeHistory;
    mutable std::atomic<size_t> historyIndex;
};

} // namespace daw::core::utilities

