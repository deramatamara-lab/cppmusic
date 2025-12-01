/**
 * @file AsyncDispatcher.hpp
 * @brief Lock-free multi-producer single-consumer queue for async dispatch
 *
 * Central async dispatcher for parameter changes, CRDT operations, and
 * other cross-thread communication without blocking the real-time audio thread.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace cppmusic::engine::concurrency {

/**
 * @brief Message priority levels
 */
enum class Priority : uint8_t {
    Low = 0,
    Normal = 1,
    High = 2,
    RealTime = 3  // For audio thread only
};

/**
 * @brief Message types for the dispatcher
 */
enum class MessageType : uint16_t {
    ParameterChange,
    CRDTOperation,
    UIUpdate,
    PerformanceMetric,
    AudioCallback,
    Custom
};

/**
 * @brief Base message structure for async dispatch
 */
struct Message {
    MessageType type{MessageType::Custom};
    Priority priority{Priority::Normal};
    uint32_t sourceId{0};
    uint32_t targetId{0};
    uint64_t timestamp{0};
    
    // Payload (up to 56 bytes inline, larger payloads use heap)
    static constexpr size_t MaxInlinePayload = 56;
    uint8_t inlinePayload[MaxInlinePayload]{};
    size_t payloadSize{0};
    void* heapPayload{nullptr};
    
    Message() = default;
    ~Message();
    
    Message(const Message& other);
    Message& operator=(const Message& other);
    Message(Message&& other) noexcept;
    Message& operator=(Message&& other) noexcept;
    
    /**
     * @brief Set payload data
     */
    template<typename T>
    void setPayload(const T& data) {
        static_assert(std::is_trivially_copyable_v<T>, "Payload must be trivially copyable");
        if constexpr (sizeof(T) <= MaxInlinePayload) {
            std::memcpy(inlinePayload, &data, sizeof(T));
            payloadSize = sizeof(T);
        } else {
            allocateHeapPayload(sizeof(T));
            std::memcpy(heapPayload, &data, sizeof(T));
            payloadSize = sizeof(T);
        }
    }
    
    /**
     * @brief Get payload data
     */
    template<typename T>
    std::optional<T> getPayload() const {
        static_assert(std::is_trivially_copyable_v<T>, "Payload must be trivially copyable");
        if (payloadSize < sizeof(T)) {
            return std::nullopt;
        }
        T result;
        const void* src = (payloadSize <= MaxInlinePayload) ? inlinePayload : heapPayload;
        std::memcpy(&result, src, sizeof(T));
        return result;
    }
    
private:
    void allocateHeapPayload(size_t size);
    void freeHeapPayload();
    void copyFrom(const Message& other);
};

/**
 * @brief Lock-free SPSC (Single Producer Single Consumer) queue
 *
 * Wait-free for producer, lock-free for consumer.
 * Suitable for audio thread → worker thread communication.
 */
template<typename T, size_t Capacity = 1024>
class SPSCQueue {
public:
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    
    SPSCQueue() : buffer_(Capacity) {}
    
    /**
     * @brief Try to push an item (wait-free)
     * @return true if successful, false if queue is full
     */
    bool tryPush(T item) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next = (head + 1) & (Capacity - 1);
        
        if (next == tail_.load(std::memory_order_acquire)) {
            return false;  // Full
        }
        
        buffer_[head] = std::move(item);
        head_.store(next, std::memory_order_release);
        return true;
    }
    
    /**
     * @brief Try to pop an item (lock-free)
     * @return Item if available, nullopt if queue is empty
     */
    std::optional<T> tryPop() {
        size_t tail = tail_.load(std::memory_order_relaxed);
        
        if (tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt;  // Empty
        }
        
        T item = std::move(buffer_[tail]);
        tail_.store((tail + 1) & (Capacity - 1), std::memory_order_release);
        return item;
    }
    
    /**
     * @brief Check if queue is empty
     */
    [[nodiscard]] bool empty() const {
        return head_.load(std::memory_order_acquire) == 
               tail_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Get approximate size
     */
    [[nodiscard]] size_t size() const {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return (head >= tail) ? (head - tail) : (Capacity - tail + head);
    }
    
private:
    std::vector<T> buffer_;
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
};

/**
 * @brief Lock-free MPSC (Multi Producer Single Consumer) queue
 *
 * Lock-free for multiple producers, wait-free for consumer.
 * Suitable for worker threads → audio thread communication.
 */
template<typename T, size_t Capacity = 1024>
class MPSCQueue {
public:
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    
    MPSCQueue() : buffer_(Capacity) {
        for (size_t i = 0; i < Capacity; ++i) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }
    
    /**
     * @brief Try to push an item (lock-free, multiple producers)
     * @return true if successful, false if queue is full
     */
    bool tryPush(T item) {
        Cell* cell;
        size_t pos = enqueuePos_.load(std::memory_order_relaxed);
        
        for (;;) {
            cell = &buffer_[pos & (Capacity - 1)];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            
            if (diff == 0) {
                if (enqueuePos_.compare_exchange_weak(pos, pos + 1,
                    std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                return false;  // Full
            } else {
                pos = enqueuePos_.load(std::memory_order_relaxed);
            }
        }
        
        cell->data = std::move(item);
        cell->sequence.store(pos + 1, std::memory_order_release);
        return true;
    }
    
    /**
     * @brief Try to pop an item (wait-free, single consumer)
     * @return Item if available, nullopt if queue is empty
     */
    std::optional<T> tryPop() {
        Cell* cell;
        size_t pos = dequeuePos_.load(std::memory_order_relaxed);
        
        cell = &buffer_[pos & (Capacity - 1)];
        size_t seq = cell->sequence.load(std::memory_order_acquire);
        intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
        
        if (diff < 0) {
            return std::nullopt;  // Empty
        }
        
        dequeuePos_.store(pos + 1, std::memory_order_relaxed);
        T item = std::move(cell->data);
        cell->sequence.store(pos + Capacity, std::memory_order_release);
        return item;
    }
    
    /**
     * @brief Check if queue is empty
     */
    [[nodiscard]] bool empty() const {
        size_t pos = dequeuePos_.load(std::memory_order_relaxed);
        Cell* cell = &buffer_[pos & (Capacity - 1)];
        size_t seq = cell->sequence.load(std::memory_order_acquire);
        return static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1) < 0;
    }
    
private:
    struct Cell {
        std::atomic<size_t> sequence;
        T data;
    };
    
    mutable std::vector<Cell> buffer_;
    alignas(64) std::atomic<size_t> enqueuePos_{0};
    alignas(64) std::atomic<size_t> dequeuePos_{0};
};

/**
 * @brief Async dispatcher for cross-thread communication
 *
 * Provides a central hub for async message passing between threads
 * without blocking the real-time audio thread.
 *
 * Thread topology:
 * - Audio thread: Real-time, highest priority, produces parameter values
 * - GUI thread: User interactions, produces control messages
 * - Worker pool: AI/Analysis tasks, produces async results
 * - Network thread: Collaboration, produces CRDT operations
 */
class AsyncDispatcher {
public:
    using Handler = std::function<void(const Message&)>;
    
    AsyncDispatcher();
    ~AsyncDispatcher();

    // Non-copyable
    AsyncDispatcher(const AsyncDispatcher&) = delete;
    AsyncDispatcher& operator=(const AsyncDispatcher&) = delete;

    /**
     * @brief Post a message to the dispatcher (thread-safe)
     * @param msg Message to post
     * @return true if queued successfully
     */
    bool post(Message msg);

    /**
     * @brief Post a message with type and payload
     */
    template<typename T>
    bool post(MessageType type, const T& payload, Priority priority = Priority::Normal) {
        Message msg;
        msg.type = type;
        msg.priority = priority;
        msg.setPayload(payload);
        return post(std::move(msg));
    }

    /**
     * @brief Process pending messages (call from consumer thread)
     * @param maxMessages Maximum messages to process (0 = unlimited)
     * @return Number of messages processed
     */
    size_t process(size_t maxMessages = 0);

    /**
     * @brief Register a handler for a message type
     * @param type Message type to handle
     * @param handler Callback function
     */
    void registerHandler(MessageType type, Handler handler);

    /**
     * @brief Unregister handler for a message type
     */
    void unregisterHandler(MessageType type);

    /**
     * @brief Get number of pending messages
     */
    [[nodiscard]] size_t pendingCount() const;

    /**
     * @brief Check if there are pending messages
     */
    [[nodiscard]] bool hasPending() const;

    /**
     * @brief Get statistics
     */
    struct Stats {
        uint64_t messagesPosted{0};
        uint64_t messagesProcessed{0};
        uint64_t messagesDropped{0};
        uint64_t queueOverflows{0};
    };
    [[nodiscard]] Stats getStats() const;

    /**
     * @brief Reset statistics
     */
    void resetStats();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace cppmusic::engine::concurrency
