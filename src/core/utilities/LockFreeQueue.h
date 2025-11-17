#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <array>
#include <type_traits>

namespace daw::core::utilities
{

/**
 * @brief Lock-free single-producer-single-consumer (SPSC) queue
 * 
 * Thread-safe queue for passing data between UI and audio threads.
 * Follows DAW_DEV_RULES: lock-free, no allocations, real-time safe.
 * 
 * @tparam T Element type (must be trivially copyable)
 * @tparam Size Queue capacity (must be power of 2)
 */
template<typename T, size_t Size>
class LockFreeQueue
{
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    static_assert(Size > 0, "Size must be greater than 0");

public:
    LockFreeQueue() noexcept
        : writePos(0)
        , readPos(0)
    {
        static_assert(alignof(T) <= alignof(std::max_align_t), "T alignment too large");
    }

    ~LockFreeQueue() = default;

    // Non-copyable, movable
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;
    LockFreeQueue(LockFreeQueue&&) noexcept = default;
    LockFreeQueue& operator=(LockFreeQueue&&) noexcept = default;

    /**
     * @brief Try to push an element (producer thread only)
     * @param element Element to push
     * @return true if successful, false if queue is full
     */
    [[nodiscard]] bool tryPush(const T& element) noexcept
    {
        const auto currentWrite = writePos.load(std::memory_order_relaxed);
        const auto nextWrite = (currentWrite + 1) & mask;
        
        // Check if queue is full
        if (nextWrite == readPos.load(std::memory_order_acquire))
            return false;
        
        buffer[currentWrite] = element;
        writePos.store(nextWrite, std::memory_order_release);
        return true;
    }

    /**
     * @brief Try to pop an element (consumer thread only)
     * @param element Reference to store popped element
     * @return true if successful, false if queue is empty
     */
    [[nodiscard]] bool tryPop(T& element) noexcept
    {
        const auto currentRead = readPos.load(std::memory_order_relaxed);
        
        // Check if queue is empty
        if (currentRead == writePos.load(std::memory_order_acquire))
            return false;
        
        element = buffer[currentRead];
        readPos.store((currentRead + 1) & mask, std::memory_order_release);
        return true;
    }

    /**
     * @brief Check if queue is empty (approximate, for statistics only)
     */
    [[nodiscard]] bool isEmpty() const noexcept
    {
        return readPos.load(std::memory_order_acquire) == writePos.load(std::memory_order_acquire);
    }

    /**
     * @brief Get approximate size (for statistics only)
     */
    [[nodiscard]] size_t size() const noexcept
    {
        const auto w = writePos.load(std::memory_order_acquire);
        const auto r = readPos.load(std::memory_order_acquire);
        return (w - r) & mask;
    }

    /**
     * @brief Get maximum capacity
     */
    [[nodiscard]] static constexpr size_t capacity() noexcept
    {
        return Size - 1; // One slot reserved for full/empty detection
    }

    /**
     * @brief Clear the queue (not thread-safe, use with caution)
     */
    void clear() noexcept
    {
        writePos.store(0, std::memory_order_relaxed);
        readPos.store(0, std::memory_order_relaxed);
    }

private:
    static constexpr size_t mask = Size - 1;
    
    alignas(64) std::array<T, Size> buffer; // Cache-aligned to avoid false sharing
    alignas(64) std::atomic<size_t> writePos; // Producer position
    alignas(64) std::atomic<size_t> readPos;  // Consumer position
};

} // namespace daw::core::utilities

