#pragma once

#include <cstdint>
#include <atomic>
#include <algorithm>

namespace daw::audio::engine
{

/**
 * @brief Dynamic block size optimizer
 * 
 * Adjusts block size based on CPU load to maintain low latency
 * while preventing xruns. Follows DAW_DEV_RULES: real-time safe.
 */
class BlockSizeOptimizer
{
public:
    BlockSizeOptimizer() noexcept
        : currentBlockSize(128)
        , minBlockSize(64)
        , maxBlockSize(512)
        , targetCpuLoad(0.75f)
        , cpuLoadThresholdHigh(0.85f)
        , cpuLoadThresholdLow(0.60f)
    {
    }

    ~BlockSizeOptimizer() = default;

    // Non-copyable, movable
    BlockSizeOptimizer(const BlockSizeOptimizer&) = delete;
    BlockSizeOptimizer& operator=(const BlockSizeOptimizer&) = delete;
    BlockSizeOptimizer(BlockSizeOptimizer&&) noexcept = default;
    BlockSizeOptimizer& operator=(BlockSizeOptimizer&&) noexcept = default;

    /**
     * @brief Update block size based on CPU load
     * @param currentCpuLoad Current CPU load (0.0 to 1.0+)
     * @return Recommended block size
     */
    [[nodiscard]] int updateBlockSize(float currentCpuLoad) noexcept
    {
        // If CPU load is too high, increase block size (reduce CPU pressure)
        if (currentCpuLoad > cpuLoadThresholdHigh)
        {
            currentBlockSize = std::min(currentBlockSize * 2, maxBlockSize);
        }
        // If CPU load is low, decrease block size (reduce latency)
        else if (currentCpuLoad < cpuLoadThresholdLow)
        {
            currentBlockSize = std::max(currentBlockSize / 2, minBlockSize);
        }
        
        return currentBlockSize;
    }

    /**
     * @brief Get current block size
     */
    [[nodiscard]] int getCurrentBlockSize() const noexcept
    {
        return currentBlockSize;
    }

    /**
     * @brief Set block size constraints
     */
    void setConstraints(int minSize, int maxSize) noexcept
    {
        minBlockSize = std::max(32, minSize);
        maxBlockSize = std::min(2048, maxSize);
        currentBlockSize = std::clamp(currentBlockSize, minBlockSize, maxBlockSize);
    }

    /**
     * @brief Set CPU load thresholds
     */
    void setThresholds(float low, float high) noexcept
    {
        cpuLoadThresholdLow = std::clamp(low, 0.0f, 1.0f);
        cpuLoadThresholdHigh = std::clamp(high, 0.0f, 1.0f);
    }

    /**
     * @brief Reset to default block size
     */
    void reset() noexcept
    {
        currentBlockSize = 128;
    }

private:
    int currentBlockSize;
    int minBlockSize;
    int maxBlockSize;
    float targetCpuLoad;
    float cpuLoadThresholdHigh;
    float cpuLoadThresholdLow;
};

} // namespace daw::audio::engine

