#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <array>
#include <cstring>

namespace daw::core {

/**
 * Lock-free, wait-free message queue for real-time audio thread communication.
 *
 * CRITICAL: This queue is designed for audio thread safety:
 * - No allocations, locks, or blocking operations
 * - Single producer, single consumer (SPSC) design
 * - Fixed-size message slots with compile-time capacity
 * - Memory ordering guarantees for cross-thread visibility
 *
 * Usage:
 * - UI/AI threads: push() messages (non-blocking, may fail if full)
 * - Audio thread: pop() messages in processBlock() (always succeeds or returns false)
 */
template<typename MessageType, size_t QueueSize = 1024>
class RealtimeMessageQueue
{
public:
    static_assert((QueueSize & (QueueSize - 1)) == 0, "QueueSize must be power of 2");
    static_assert(std::is_trivially_copyable_v<MessageType>, "MessageType must be trivially copyable");
    static_assert(sizeof(MessageType) <= 256, "MessageType should be small for cache efficiency");

    RealtimeMessageQueue() noexcept
        : writeIndex(0)
        , readIndex(0)
    {
        // Initialize all slots to ensure deterministic state
        for (auto& slot : slots)
        {
            std::memset(&slot, 0, sizeof(slot));
        }
    }

    /**
     * Push a message to the queue (called from UI/AI threads).
     *
     * @param message The message to push
     * @return true if message was pushed, false if queue was full
     *
     * This is wait-free and will never block the caller.
     */
    [[nodiscard]] bool push(const MessageType& message) noexcept
    {
        const auto currentWrite = writeIndex.load(std::memory_order_relaxed);
        const auto nextWrite = (currentWrite + 1) & (QueueSize - 1);

        // Check if queue is full
        if (nextWrite == readIndex.load(std::memory_order_acquire))
        {
            return false; // Queue full, drop message
        }

        // Copy message to slot
        slots[currentWrite] = message;

        // Make message visible to reader
        writeIndex.store(nextWrite, std::memory_order_release);
        return true;
    }

    /**
     * Pop a message from the queue (called from audio thread).
     *
     * @param message Output parameter to receive the message
     * @return true if message was popped, false if queue was empty
     *
     * This is wait-free and safe to call from processBlock().
     */
    [[nodiscard]] bool pop(MessageType& message) noexcept
    {
        const auto currentRead = readIndex.load(std::memory_order_relaxed);

        // Check if queue is empty
        if (currentRead == writeIndex.load(std::memory_order_acquire))
        {
            return false; // Queue empty
        }

        // Copy message from slot
        message = slots[currentRead];

        // Advance read position
        const auto nextRead = (currentRead + 1) & (QueueSize - 1);
        readIndex.store(nextRead, std::memory_order_release);
        return true;
    }

    /**
     * Get current queue usage statistics.
     * Safe to call from any thread.
     */
    struct Stats
    {
        size_t size;
        size_t capacity;
        float utilization; // 0.0 to 1.0
    };

    [[nodiscard]] Stats getStats() const noexcept
    {
        const auto write = writeIndex.load(std::memory_order_acquire);
        const auto read = readIndex.load(std::memory_order_acquire);

        const size_t size = (write >= read) ?
            (write - read) :
            (QueueSize - read + write);

        return Stats{
            size,
            QueueSize,
            static_cast<float>(size) / static_cast<float>(QueueSize)
        };
    }

    /**
     * Check if queue is empty (may be stale by the time you use it).
     */
    [[nodiscard]] bool empty() const noexcept
    {
        return readIndex.load(std::memory_order_acquire) ==
               writeIndex.load(std::memory_order_acquire);
    }

    /**
     * Clear all messages (not real-time safe, use only during initialization).
     */
    void clear() noexcept
    {
        readIndex.store(0, std::memory_order_release);
        writeIndex.store(0, std::memory_order_release);
    }

private:
    // Cache-aligned atomic indices
    alignas(64) std::atomic<size_t> writeIndex;
    alignas(64) std::atomic<size_t> readIndex;

    // Message storage
    std::array<MessageType, QueueSize> slots;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealtimeMessageQueue)
};

/**
 * Common message types for DAW communication
 */
namespace Messages {

    struct ParameterChange
    {
        uint32_t parameterId;
        float value;
        uint64_t timestamp;
    };

    struct AIResult
    {
        enum Type : uint8_t {
            ChordSuggestion,
            MelodyGeneration,
            BeatAnalysis,
            GrooveExtraction
        };

        Type type;
        uint32_t requestId;
        float confidence;
        std::array<float, 16> data; // Small fixed-size payload
    };

    struct TransportCommand
    {
        enum Command : uint8_t {
            Play,
            Stop,
            Pause,
            Record,
            SetPosition
        };

        Command command;
        double positionSeconds;
        uint64_t timestamp;
    };

    struct MeterUpdate
    {
        uint32_t channelId;
        float peakLevel;
        float rmsLevel;
        uint64_t timestamp;
    };

} // namespace Messages

// Common queue type aliases
using ParameterQueue = RealtimeMessageQueue<Messages::ParameterChange, 512>;
using AIResultQueue = RealtimeMessageQueue<Messages::AIResult, 256>;
using TransportQueue = RealtimeMessageQueue<Messages::TransportCommand, 64>;
using MeterQueue = RealtimeMessageQueue<Messages::MeterUpdate, 1024>;

} // namespace daw::core
