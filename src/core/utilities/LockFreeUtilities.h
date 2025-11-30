#pragma once

#include <atomic>
#include <array>
#include <memory>
#include <type_traits>
#include <cstdint>
#include <cstring>
#include <new>

namespace daw::core::utilities {

/**
 * @brief Lock-free, wait-free ring buffer for single producer, single consumer
 *
 * Designed for real-time audio applications with:
 * - Zero allocations in hot paths
 * - Memory ordering guarantees
 * - Power-of-two capacity for efficient indexing
 * - Trivially copyable element types only
 */
template<typename T, size_t Capacity>
class LockFreeRingBuffer
{
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

public:
    LockFreeRingBuffer() noexcept
        : writeIndex_(0)
        , readIndex_(0)
    {
        // Ensure all elements are initialized
        for (auto& elem : buffer_)
        {
            std::memset(&elem, 0, sizeof(T));
        }
    }

    /**
     * @brief Push an element to the buffer
     * @param value The value to push
     * @return true if successful, false if buffer was full
     */
    bool push(const T& value) noexcept
    {
        const size_t writeIdx = writeIndex_.load(std::memory_order_relaxed);
        const size_t nextWriteIdx = (writeIdx + 1) & (Capacity - 1);

        // Check if buffer is full
        if (nextWriteIdx == readIndex_.load(std::memory_order_acquire))
        {
            return false; // Buffer full
        }

        // Write the value
        buffer_[writeIdx] = value;

        // Make write visible to consumer
        writeIndex_.store(nextWriteIdx, std::memory_order_release);
        return true;
    }

    /**
     * @brief Pop an element from the buffer
     * @param value Output parameter for the popped value
     * @return true if successful, false if buffer was empty
     */
    bool pop(T& value) noexcept
    {
        const size_t readIdx = readIndex_.load(std::memory_order_relaxed);

        // Check if buffer is empty
        if (readIdx == writeIndex_.load(std::memory_order_acquire))
        {
            return false; // Buffer empty
        }

        // Read the value
        value = buffer_[readIdx];

        // Advance read index
        const size_t nextReadIdx = (readIdx + 1) & (Capacity - 1);
        readIndex_.store(nextReadIdx, std::memory_order_release);
        return true;
    }

    /**
     * @brief Get current buffer statistics
     */
    struct Stats
    {
        size_t size;
        size_t capacity;
        bool empty;
        bool full;
    };

    Stats getStats() const noexcept
    {
        const size_t writeIdx = writeIndex_.load(std::memory_order_acquire);
        const size_t readIdx = readIndex_.load(std::memory_order_acquire);

        size_t size;
        if (writeIdx >= readIdx)
        {
            size = writeIdx - readIdx;
        }
        else
        {
            size = Capacity - readIdx + writeIdx;
        }

        return Stats{
            size,
            Capacity,
            size == 0,
            size == Capacity
        };
    }

    /**
     * @brief Clear the buffer (not real-time safe)
     */
    void clear() noexcept
    {
        writeIndex_.store(0, std::memory_order_release);
        readIndex_.store(0, std::memory_order_release);
    }

private:
    alignas(64) std::atomic<size_t> writeIndex_;
    alignas(64) std::atomic<size_t> readIndex_;
    std::array<T, Capacity> buffer_;
};

/**
 * @brief Real-time memory pool for zero-allocation audio processing
 *
 * Provides pre-allocated memory pools for common audio processing needs:
 * - Temporary buffers for processing
 * - Scratch space for algorithms
 * - Fixed-size allocations to avoid fragmentation
 *
 * Thread-safe for single writer, multiple readers pattern.
 */
class RTMemoryPool
{
public:
    /**
     * @brief Memory block descriptor
     */
    struct Block
    {
        void* data;
        size_t size;
        size_t alignment;

        Block() noexcept : data(nullptr), size(0), alignment(0) {}
        Block(void* d, size_t s, size_t a) noexcept : data(d), size(s), alignment(a) {}

        template<typename T>
        T* as() const noexcept { return static_cast<T*>(data); }
    };

    explicit RTMemoryPool(size_t totalSizeBytes = 1024 * 1024); // 1MB default
    ~RTMemoryPool();

    // Non-copyable, movable
    RTMemoryPool(const RTMemoryPool&) = delete;
    RTMemoryPool& operator=(const RTMemoryPool&) = delete;
    RTMemoryPool(RTMemoryPool&&) noexcept = default;
    RTMemoryPool& operator=(RTMemoryPool&&) noexcept = default;

    /**
     * @brief Allocate a block of memory
     * @param size Size in bytes
     * @param alignment Alignment requirement (must be power of 2)
     * @return Block descriptor, or empty block if allocation failed
     */
    Block allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    /**
     * @brief Deallocate a previously allocated block
     * @param block The block to deallocate
     */
    void deallocate(const Block& block);

    /**
     * @brief Get pool statistics
     */
    struct Stats
    {
        size_t totalSize;
        size_t usedSize;
        size_t freeSize;
        size_t allocationCount;
        float utilization; // 0.0 to 1.0
    };

    Stats getStats() const noexcept;

    /**
     * @brief Reset pool (not real-time safe, use during initialization)
     */
    void reset();

private:
    struct FreeBlock
    {
        void* data;
        size_t size;
        FreeBlock* next;
    };

    void* poolStart_;
    size_t poolSize_;
    FreeBlock* freeList_;
    size_t allocationCount_;
    mutable std::atomic<size_t> usedSize_;

    void initializeFreeList();
};

/**
 * @brief Sample-accurate transport state tracker
 *
 * Tracks transport position with sample-level accuracy for:
 * - Tempo changes
 * - Time signature changes
 * - Loop points
 * - Position jumps
 */
class SampleAccurateTransport
{
public:
    struct Position
    {
        double beats;           // Position in beats
        double samples;         // Position in samples
        double seconds;         // Position in seconds
        int bar;               // Current bar (1-based)
        int beat;              // Current beat in bar (1-based)
        int sixteenth;         // Current sixteenth in beat (1-based)
    };

    SampleAccurateTransport();

    /**
     * @brief Update transport state
     * @param samplesProcessed Number of samples processed since last update
     * @param sampleRate Current sample rate
     */
    void update(size_t samplesProcessed, double sampleRate) noexcept;

    /**
     * @brief Set tempo
     * @param bpm New tempo in BPM
     */
    void setTempo(double bpm) noexcept;

    /**
     * @brief Set time signature
     * @param numerator Time signature numerator
     * @param denominator Time signature denominator
     */
    void setTimeSignature(int numerator, int denominator) noexcept;

    /**
     * @brief Set transport position
     * @param beats Position in beats
     */
    void setPositionInBeats(double beats) noexcept;

    /**
     * @brief Start transport
     */
    void start() noexcept;

    /**
     * @brief Stop transport
     */
    void stop() noexcept;

    /**
     * @brief Check if transport is running
     */
    bool isPlaying() const noexcept;

    /**
     * @brief Get current position
     */
    Position getCurrentPosition() const noexcept;

    /**
     * @brief Get tempo
     */
    double getTempo() const noexcept;

    /**
     * @brief Get time signature
     */
    std::pair<int, int> getTimeSignature() const noexcept;

private:
    std::atomic<double> tempo_;           // BPM
    std::atomic<int> timeSigNumerator_;  // Time signature numerator
    std::atomic<int> timeSigDenominator_; // Time signature denominator
    std::atomic<bool> isPlaying_;         // Transport running state

    // Position tracking (updated atomically)
    alignas(64) mutable std::atomic<double> currentBeats_;
    double accumulatedSamples_;           // Non-atomic accumulator

    // Position calculation helpers
    double samplesPerBeat() const noexcept;
    int calculateBar(double beats) const noexcept;
    int calculateBeatInBar(double beats) const noexcept;
    int calculateSixteenthInBeat(double beats) const noexcept;
};

/**
 * @brief Lock-free atomic flag with additional operations
 */
class AtomicFlag
{
public:
    AtomicFlag() noexcept : flag_(false) {}
    explicit AtomicFlag(bool initial) noexcept : flag_(initial) {}

    /**
     * @brief Set flag to true
     * @return Previous value
     */
    bool set() noexcept
    {
        return flag_.exchange(true, std::memory_order_acq_rel);
    }

    /**
     * @brief Set flag to false
     * @return Previous value
     */
    bool clear() noexcept
    {
        return flag_.exchange(false, std::memory_order_acq_rel);
    }

    /**
     * @brief Test and set (atomic)
     * @return true if flag was previously false
     */
    bool testAndSet() noexcept
    {
        bool expected = false;
        return flag_.compare_exchange_strong(expected, true,
                                           std::memory_order_acq_rel,
                                           std::memory_order_acquire);
    }

    /**
     * @brief Test flag value
     */
    bool test() const noexcept
    {
        return flag_.load(std::memory_order_acquire);
    }

    /**
     * @brief Exchange flag value
     */
    bool exchange(bool value) noexcept
    {
        return flag_.exchange(value, std::memory_order_acq_rel);
    }

private:
    std::atomic<bool> flag_;
};

/**
 * @brief Thread-safe statistics accumulator for performance monitoring
 */
template<typename T>
class AtomicStatistics
{
public:
    AtomicStatistics() noexcept
        : count_(0)
        , sum_(0)
        , min_(std::numeric_limits<T>::max())
        , max_(std::numeric_limits<T>::min())
    {}

    /**
     * @brief Add a sample to the statistics
     */
    void addSample(T sample) noexcept
    {
        count_.fetch_add(1, std::memory_order_relaxed);
        sum_.fetch_add(sample, std::memory_order_relaxed);

        // Update min/max atomically
        T currentMin = min_.load(std::memory_order_relaxed);
        while (sample < currentMin &&
               !min_.compare_exchange_weak(currentMin, sample,
                                          std::memory_order_acq_rel,
                                          std::memory_order_relaxed))
        {
            // Loop until successful or sample is not smaller
        }

        T currentMax = max_.load(std::memory_order_relaxed);
        while (sample > currentMax &&
               !max_.compare_exchange_weak(currentMax, sample,
                                          std::memory_order_acq_rel,
                                          std::memory_order_relaxed))
        {
            // Loop until successful or sample is not larger
        }
    }

    /**
     * @brief Get current statistics
     */
    struct Stats
    {
        size_t count;
        T sum;
        T min;
        T max;
        T average;
    };

    Stats getStats() const noexcept
    {
        const size_t count = count_.load(std::memory_order_acquire);
        const T sum = sum_.load(std::memory_order_acquire);

        return Stats{
            count,
            sum,
            min_.load(std::memory_order_acquire),
            max_.load(std::memory_order_acquire),
            count > 0 ? static_cast<T>(sum) / static_cast<T>(count) : 0
        };
    }

    /**
     * @brief Reset statistics
     */
    void reset() noexcept
    {
        count_.store(0, std::memory_order_release);
        sum_.store(0, std::memory_order_release);
        min_.store(std::numeric_limits<T>::max(), std::memory_order_release);
        max_.store(std::numeric_limits<T>::min(), std::memory_order_release);
    }

private:
    std::atomic<size_t> count_;
    std::atomic<T> sum_;
    std::atomic<T> min_;
    std::atomic<T> max_;
};

} // namespace daw::core::utilities
