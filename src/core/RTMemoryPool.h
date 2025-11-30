#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>
#include <vector>
#include <cstddef>
#include <chrono>

namespace daw::core {

/**
 * Real-Time Memory Pool for zero-allocation audio processing.
 *
 * CRITICAL: This class enforces the "No heap allocations in audio thread" rule
 * by providing pre-allocated memory blocks that can be safely used from the
 * audio processing thread without blocking or allocating.
 *
 * Features:
 * - Lock-free allocation/deallocation for audio thread
 * - Cache-aligned memory blocks for optimal performance
 * - Comprehensive statistics and monitoring
 * - Automatic fragmentation management
 * - Platform-specific optimizations
 */
class RTMemoryPool
{
public:
    /**
     * Memory block header structure
     */
    struct alignas(64) MemoryBlock
    {
        std::atomic<MemoryBlock*> next{nullptr};
        std::atomic<size_t> size{0};
        std::atomic<bool> isAllocated{false};
        std::atomic<uint32_t> refCount{0};
        std::atomic<uint64_t> allocationTime{0}; // Microseconds since epoch

        // Actual data follows after this header
        [[nodiscard]] void* getData() noexcept
        {
            return reinterpret_cast<char*>(this) + sizeof(MemoryBlock);
        }
    };

    /**
     * Pool configuration
     */
    struct PoolConfig
    {
        size_t blockSize = 4096;           // Size of each block in bytes
        size_t numBlocks = 512;            // Initial number of blocks
        size_t maxPoolSize = 32 * 1024 * 1024; // Maximum pool size (32MB)
        size_t alignment = 64;             // Memory alignment (cache line)
        bool enableStatistics = true;     // Track allocation statistics
        bool enableMemoryLocking = true;  // Lock memory to prevent page faults
        bool enableFallback = false;      // Allow fallback to heap (not RT-safe)
    };

    /**
     * Memory statistics for monitoring
     */
    struct MemoryStats
    {
        std::atomic<size_t> totalAllocated{0};
        std::atomic<size_t> totalFreed{0};
        std::atomic<size_t> currentUsage{0};
        std::atomic<size_t> peakUsage{0};
        std::atomic<size_t> allocationCount{0};
        std::atomic<size_t> deallocationCount{0};
        std::atomic<size_t> fallbackAllocations{0};
        std::atomic<size_t> fragmentedBlocks{0};
        std::atomic<float> fragmentationRatio{0.0f};

        // Performance metrics
        std::atomic<uint64_t> averageAllocationTimeUs{0};
        std::atomic<uint64_t> maxAllocationTimeUs{0};
        std::atomic<uint64_t> totalAllocationTimeUs{0};
    };

    explicit RTMemoryPool() noexcept;
    explicit RTMemoryPool(const PoolConfig& config) noexcept;
    ~RTMemoryPool();

    //==============================================================================
    // Real-Time Safe Operations (Audio Thread)
    //==============================================================================

    /**
     * Allocate memory block (real-time safe, no locks)
     * @param size Size in bytes to allocate
     * @return Memory pointer, or nullptr if allocation failed
     */
    [[nodiscard]] void* allocate(size_t size) noexcept;

    /**
     * Allocate aligned memory block (real-time safe)
     * @param size Size in bytes to allocate
     * @param alignment Memory alignment requirement
     * @return Aligned memory pointer, or nullptr if allocation failed
     */
    [[nodiscard]] void* allocateAligned(size_t size, size_t alignment) noexcept;

    /**
     * Deallocate memory block (real-time safe, no locks)
     * @param ptr Memory pointer to deallocate
     */
    void deallocate(void* ptr) noexcept;

    //==============================================================================
    // Thread-Safe Operations (Non-Audio Threads)
    //==============================================================================

    /**
     * Thread-safe allocation with mutex protection
     * @param size Size in bytes to allocate
     * @return Memory pointer, or nullptr if allocation failed
     */
    [[nodiscard]] void* allocateThreadSafe(size_t size);

    /**
     * Thread-safe deallocation with mutex protection
     * @param ptr Memory pointer to deallocate
     */
    void deallocateThreadSafe(void* ptr);

    //==============================================================================
    // Pool Management
    //==============================================================================

    /**
     * Initialize the memory pool
     * @return true if initialization succeeded
     */
    bool initialize();

    /**
     * Shutdown and cleanup the memory pool
     */
    void shutdown();

    /**
     * Reset the pool (deallocate all blocks)
     */
    void reset();

    /**
     * Check if pool is initialized and ready for use
     */
    [[nodiscard]] bool isReady() const noexcept { return initialized.load(); }

    //==============================================================================
    // Statistics and Monitoring
    //==============================================================================

    /**
     * Get current memory statistics
     */
    const MemoryStats& getStats() const noexcept { return stats; }

    /**
     * Get number of available blocks
     */
    [[nodiscard]] size_t getAvailableBlocks() const noexcept;

    /**
     * Get total number of blocks in pool
     */
    [[nodiscard]] size_t getTotalBlocks() const noexcept { return config.numBlocks; }

    /**
     * Get current fragmentation ratio (0.0 to 1.0)
     */
    [[nodiscard]] float getFragmentationRatio() const noexcept;

    /**
     * Check if pool is exhausted (no free blocks)
     */
    [[nodiscard]] bool isPoolExhausted() const noexcept;

    //==============================================================================
    // Debug and Diagnostics
    //==============================================================================

    /**
     * Validate memory pool integrity
     * @return true if pool is in valid state
     */
    [[nodiscard]] bool validateIntegrity() const;

    /**
     * Log current memory statistics
     */
    void logStats() const;

private:
    PoolConfig config;
    mutable MemoryStats stats;

    // Pool state
    std::unique_ptr<char[]> poolMemory;
    std::atomic<MemoryBlock*> freeList{nullptr};
    std::atomic<MemoryBlock*> allocatedList{nullptr};
    std::atomic<size_t> totalPoolSizeBytes{0};
    std::atomic<bool> initialized{false};

    // Thread safety for non-RT operations
    mutable std::mutex poolMutex;

    //==============================================================================
    // Internal Methods
    //==============================================================================

    MemoryBlock* findFreeBlock(size_t size) noexcept;
    void returnBlockToFreeList(MemoryBlock* block) noexcept;
    void updateStatistics(size_t size, uint64_t allocationTimeUs) noexcept;
    void calculateFragmentation() noexcept;

    // Memory utilities
    [[nodiscard]] static size_t alignSize(size_t size, size_t alignment) noexcept;
    [[nodiscard]] static bool isPowerOfTwo(size_t value) noexcept;
    [[nodiscard]] static size_t nextPowerOfTwo(size_t value) noexcept;
    [[nodiscard]] static uint64_t getCurrentTimeUs() noexcept;

    // Platform-specific memory locking
    void lockMemoryPages(void* ptr, size_t size);
    void unlockMemoryPages(void* ptr, size_t size);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RTMemoryPool)
};

/**
 * RAII wrapper for automatic memory deallocation
 */
class RTMemoryScope
{
public:
    explicit RTMemoryScope(RTMemoryPool& pool, size_t size) noexcept
        : pool_(pool)
        , ptr_(pool.allocate(size))
        , size_(size)
    {
    }

    ~RTMemoryScope() noexcept
    {
        if (ptr_)
            pool_.deallocate(ptr_);
    }

    [[nodiscard]] void* get() const noexcept { return ptr_; }
    [[nodiscard]] bool isValid() const noexcept { return ptr_ != nullptr; }
    [[nodiscard]] size_t size() const noexcept { return size_; }

    // Non-copyable
    RTMemoryScope(const RTMemoryScope&) = delete;
    RTMemoryScope& operator=(const RTMemoryScope&) = delete;

    // Movable
    RTMemoryScope(RTMemoryScope&& other) noexcept
        : pool_(other.pool_)
        , ptr_(other.ptr_)
        , size_(other.size_)
    {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }

    RTMemoryScope& operator=(RTMemoryScope&& other) noexcept
    {
        if (this != &other)
        {
            if (ptr_)
                pool_.deallocate(ptr_);

            ptr_ = other.ptr_;
            size_ = other.size_;
            other.ptr_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

private:
    RTMemoryPool& pool_;
    void* ptr_;
    size_t size_;
};

/**
 * Template wrapper for typed memory allocation
 */
template<typename T>
class RTMemoryArray
{
public:
    RTMemoryArray(RTMemoryPool& pool, size_t count) noexcept
        : pool_(pool)
        , ptr_(static_cast<T*>(pool.allocate(count * sizeof(T))))
        , count_(ptr_ ? count : 0)
    {
        if (ptr_)
        {
            // Placement new for each element
            for (size_t i = 0; i < count_; ++i)
            {
                new (ptr_ + i) T();
            }
        }
    }

    ~RTMemoryArray() noexcept
    {
        if (ptr_)
        {
            // Explicit destructor calls
            for (size_t i = 0; i < count_; ++i)
            {
                ptr_[i].~T();
            }

            pool_.deallocate(ptr_);
        }
    }

    [[nodiscard]] T* get() const noexcept { return ptr_; }
    [[nodiscard]] T& operator[](size_t index) const noexcept
    {
        jassert(index < count_);
        return ptr_[index];
    }
    [[nodiscard]] size_t size() const noexcept { return count_; }
    [[nodiscard]] bool isValid() const noexcept { return ptr_ != nullptr; }

    // Iterator support
    [[nodiscard]] T* begin() const noexcept { return ptr_; }
    [[nodiscard]] T* end() const noexcept { return ptr_ + count_; }

    // Non-copyable, non-movable for simplicity
    RTMemoryArray(const RTMemoryArray&) = delete;
    RTMemoryArray& operator=(const RTMemoryArray&) = delete;
    RTMemoryArray(RTMemoryArray&&) = delete;
    RTMemoryArray& operator=(RTMemoryArray&&) = delete;

private:
    RTMemoryPool& pool_;
    T* ptr_;
    size_t count_;
};

} // namespace daw::core
