#pragma once

#include "JuceHeader.h"
#include <atomic>
#include <array>
#include <memory>

namespace daw::core {

/**
 * Lock-free ring buffer optimized for single-producer single-consumer scenarios.
 *
 * This implementation is specifically designed for real-time audio thread communication:
 * - No locks, mutexes, or blocking operations
 * - Memory ordering guarantees for cross-thread visibility
 * - Cache-friendly memory layout
 * - Bounded capacity with overflow handling
 * - Template-based for type safety
 */
template<typename T, size_t Capacity>
class LockFreeRingBuffer
{
public:
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    static_assert(Capacity >= 2, "Capacity must be at least 2");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for lock-free operations");

    /**
     * Buffer statistics for monitoring
     */
    struct BufferStats
    {
        std::atomic<size_t> totalWrites{0};
        std::atomic<size_t> totalReads{0};
        std::atomic<size_t> overflows{0};      // Writes when buffer was full
        std::atomic<size_t> underflows{0};     // Reads when buffer was empty
        std::atomic<size_t> maxUsage{0};       // Peak number of elements
        std::atomic<float> averageUsage{0.0f}; // Running average usage
    };

    LockFreeRingBuffer()
        : writeIndex(0)
        , readIndex(0)
    {
        // Initialize buffer to ensure deterministic state
        buffer.fill(T{});
    }

    ~LockFreeRingBuffer() = default;

    //==============================================================================
    // Producer Interface (Single Thread)
    //==============================================================================

    /**
     * Push an element to the buffer (non-blocking)
     * @param item Item to push
     * @return true if item was pushed, false if buffer was full
     */
    [[nodiscard]] bool push(const T& item) noexcept
    {
        const size_t currentWrite = writeIndex.load(std::memory_order_relaxed);
        const size_t nextWrite = (currentWrite + 1) & (Capacity - 1);

        // Check if buffer is full
        if (nextWrite == readIndex.load(std::memory_order_acquire))
        {
            stats.overflows.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        // Store the item
        buffer[currentWrite] = item;

        // Make item visible to consumer
        writeIndex.store(nextWrite, std::memory_order_release);

        // Update statistics
        stats.totalWrites.fetch_add(1, std::memory_order_relaxed);
        updateUsageStats();

        return true;
    }

    /**
     * Push an element with move semantics
     */
    [[nodiscard]] bool push(T&& item) noexcept
    {
        const size_t currentWrite = writeIndex.load(std::memory_order_relaxed);
        const size_t nextWrite = (currentWrite + 1) & (Capacity - 1);

        if (nextWrite == readIndex.load(std::memory_order_acquire))
        {
            stats.overflows.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        buffer[currentWrite] = std::move(item);
        writeIndex.store(nextWrite, std::memory_order_release);

        stats.totalWrites.fetch_add(1, std::memory_order_relaxed);
        updateUsageStats();

        return true;
    }

    /**
     * Try to push multiple elements (as many as possible)
     * @param items Array of items to push
     * @param count Number of items in array
     * @return Number of items actually pushed
     */
    [[nodiscard]] size_t pushMultiple(const T* items, size_t count) noexcept
    {
        if (!items || count == 0)
            return 0;

        size_t pushed = 0;
        for (size_t i = 0; i < count; ++i)
        {
            if (!push(items[i]))
                break;
            ++pushed;
        }

        return pushed;
    }

    //==============================================================================
    // Consumer Interface (Single Thread)
    //==============================================================================

    /**
     * Pop an element from the buffer (non-blocking)
     * @param item Output parameter to receive the item
     * @return true if item was popped, false if buffer was empty
     */
    [[nodiscard]] bool pop(T& item) noexcept
    {
        const size_t currentRead = readIndex.load(std::memory_order_relaxed);

        // Check if buffer is empty
        if (currentRead == writeIndex.load(std::memory_order_acquire))
        {
            stats.underflows.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        // Read the item
        item = buffer[currentRead];

        // Advance read position
        const size_t nextRead = (currentRead + 1) & (Capacity - 1);
        readIndex.store(nextRead, std::memory_order_release);

        stats.totalReads.fetch_add(1, std::memory_order_relaxed);

        return true;
    }

    /**
     * Peek at the next element without removing it
     * @param item Output parameter to receive the item
     * @return true if item was peeked, false if buffer was empty
     */
    [[nodiscard]] bool peek(T& item) const noexcept
    {
        const size_t currentRead = readIndex.load(std::memory_order_relaxed);

        if (currentRead == writeIndex.load(std::memory_order_acquire))
        {
            return false;
        }

        item = buffer[currentRead];
        return true;
    }

    /**
     * Pop multiple elements (as many as available)
     * @param items Array to store popped items
     * @param maxCount Maximum number of items to pop
     * @return Number of items actually popped
     */
    [[nodiscard]] size_t popMultiple(T* items, size_t maxCount) noexcept
    {
        if (!items || maxCount == 0)
            return 0;

        size_t popped = 0;
        for (size_t i = 0; i < maxCount; ++i)
        {
            if (!pop(items[i]))
                break;
            ++popped;
        }

        return popped;
    }

    //==============================================================================
    // Query Interface (Safe from any thread)
    //==============================================================================

    /**
     * Get current number of elements in buffer
     * Note: This is a snapshot and may be stale by the time you use it
     */
    [[nodiscard]] size_t size() const noexcept
    {
        const size_t write = writeIndex.load(std::memory_order_acquire);
        const size_t read = readIndex.load(std::memory_order_acquire);
        return (write >= read) ? (write - read) : (Capacity - read + write);
    }

    /**
     * Check if buffer is empty
     */
    [[nodiscard]] bool empty() const noexcept
    {
        return readIndex.load(std::memory_order_acquire) ==
               writeIndex.load(std::memory_order_acquire);
    }

    /**
     * Check if buffer is full
     */
    [[nodiscard]] bool full() const noexcept
    {
        const size_t write = writeIndex.load(std::memory_order_acquire);
        const size_t nextWrite = (write + 1) & (Capacity - 1);
        return nextWrite == readIndex.load(std::memory_order_acquire);
    }

    /**
     * Get buffer capacity
     */
    [[nodiscard]] constexpr size_t capacity() const noexcept { return Capacity; }

    /**
     * Get remaining space in buffer
     */
    [[nodiscard]] size_t available() const noexcept
    {
        return capacity() - size() - 1; // -1 because we can't completely fill the buffer
    }

    /**
     * Get buffer utilization ratio (0.0 to 1.0)
     */
    [[nodiscard]] float utilization() const noexcept
    {
        return static_cast<float>(size()) / static_cast<float>(capacity() - 1);
    }

    //==============================================================================
    // Management Interface
    //==============================================================================

    /**
     * Clear all elements (not thread-safe, use only when no other threads are accessing)
     */
    void clear() noexcept
    {
        readIndex.store(0, std::memory_order_release);
        writeIndex.store(0, std::memory_order_release);

        // Optional: clear the buffer contents for security
        buffer.fill(T{});
    }

    /**
     * Get buffer statistics
     */
    const BufferStats& getStats() const noexcept { return stats; }

    /**
     * Reset statistics (not thread-safe)
     */
    void resetStats() noexcept
    {
        stats.totalWrites.store(0);
        stats.totalReads.store(0);
        stats.overflows.store(0);
        stats.underflows.store(0);
        stats.maxUsage.store(0);
        stats.averageUsage.store(0.0f);
    }

private:
    // Cache-aligned atomic indices to prevent false sharing
    alignas(64) std::atomic<size_t> writeIndex;
    alignas(64) std::atomic<size_t> readIndex;

    // Buffer storage
    std::array<T, Capacity> buffer;

    // Statistics
    mutable BufferStats stats;

    //==============================================================================
    // Internal Methods
    //==============================================================================

    void updateUsageStats() noexcept
    {
        const size_t currentSize = size();

        // Update max usage
        size_t maxUsage = stats.maxUsage.load(std::memory_order_relaxed);
        while (currentSize > maxUsage &&
               !stats.maxUsage.compare_exchange_weak(maxUsage, currentSize, std::memory_order_relaxed))
        {
            // Retry if another thread updated max usage
        }

        // Update running average (simple exponential moving average)
        const float currentAvg = stats.averageUsage.load(std::memory_order_relaxed);
        const float newAvg = (currentAvg * 0.95f) + (static_cast<float>(currentSize) * 0.05f);
        stats.averageUsage.store(newAvg, std::memory_order_relaxed);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LockFreeRingBuffer)
};

//==============================================================================
// Common Type Aliases
//==============================================================================

// Audio sample buffer
using AudioSampleBuffer = LockFreeRingBuffer<float, 8192>;

// MIDI message buffer
using MidiMessageBuffer = LockFreeRingBuffer<juce::MidiMessage, 1024>;

// Generic message buffer for small data
template<typename T>
using MessageBuffer = LockFreeRingBuffer<T, 512>;

// Large data buffer for audio processing
template<typename T>
using LargeBuffer = LockFreeRingBuffer<T, 16384>;

//==============================================================================
// Multi-Producer Single-Consumer Ring Buffer
//==============================================================================

/**
 * Lock-free ring buffer that supports multiple producers but single consumer.
 * Uses atomic operations for thread-safe multi-producer access.
 */
template<typename T, size_t Capacity>
class MPSCRingBuffer
{
public:
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    static_assert(Capacity >= 2, "Capacity must be at least 2");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

    MPSCRingBuffer() : writeIndex(0), readIndex(0)
    {
        buffer.fill(T{});
    }

    // Multi-producer push (thread-safe)
    [[nodiscard]] bool push(const T& item) noexcept
    {
        size_t currentWrite = writeIndex.load(std::memory_order_relaxed);

        while (true)
        {
            const size_t nextWrite = (currentWrite + 1) & (Capacity - 1);

            // Check if buffer would be full
            if (nextWrite == readIndex.load(std::memory_order_acquire))
            {
                return false; // Buffer full
            }

            // Try to claim this write position
            if (writeIndex.compare_exchange_weak(currentWrite, nextWrite,
                                               std::memory_order_acq_rel,
                                               std::memory_order_relaxed))
            {
                // Successfully claimed position, write the data
                buffer[currentWrite] = item;
                return true;
            }

            // Another thread claimed this position, retry with updated currentWrite
        }
    }

    // Single-consumer pop (not thread-safe with other consumers)
    [[nodiscard]] bool pop(T& item) noexcept
    {
        const size_t currentRead = readIndex.load(std::memory_order_relaxed);

        if (currentRead == writeIndex.load(std::memory_order_acquire))
        {
            return false; // Buffer empty
        }

        item = buffer[currentRead];

        const size_t nextRead = (currentRead + 1) & (Capacity - 1);
        readIndex.store(nextRead, std::memory_order_release);

        return true;
    }

    [[nodiscard]] size_t size() const noexcept
    {
        const size_t write = writeIndex.load(std::memory_order_acquire);
        const size_t read = readIndex.load(std::memory_order_acquire);
        return (write >= read) ? (write - read) : (Capacity - read + write);
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return readIndex.load(std::memory_order_acquire) ==
               writeIndex.load(std::memory_order_acquire);
    }

    void clear() noexcept
    {
        readIndex.store(0, std::memory_order_release);
        writeIndex.store(0, std::memory_order_release);
        buffer.fill(T{});
    }

private:
    alignas(64) std::atomic<size_t> writeIndex;
    alignas(64) std::atomic<size_t> readIndex;
    std::array<T, Capacity> buffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPSCRingBuffer)
};

} // namespace daw::core
