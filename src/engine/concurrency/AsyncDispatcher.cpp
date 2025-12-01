/**
 * @file AsyncDispatcher.cpp
 * @brief Implementation of lock-free async dispatcher
 */

#include "AsyncDispatcher.hpp"

#include <chrono>
#include <mutex>
#include <unordered_map>

namespace cppmusic::engine::concurrency {

// Message implementation

Message::~Message() {
    freeHeapPayload();
}

Message::Message(const Message& other) {
    copyFrom(other);
}

Message& Message::operator=(const Message& other) {
    if (this != &other) {
        freeHeapPayload();
        copyFrom(other);
    }
    return *this;
}

Message::Message(Message&& other) noexcept
    : type(other.type)
    , priority(other.priority)
    , sourceId(other.sourceId)
    , targetId(other.targetId)
    , timestamp(other.timestamp)
    , payloadSize(other.payloadSize)
    , heapPayload(other.heapPayload) {
    std::memcpy(inlinePayload, other.inlinePayload, MaxInlinePayload);
    other.heapPayload = nullptr;
    other.payloadSize = 0;
}

Message& Message::operator=(Message&& other) noexcept {
    if (this != &other) {
        freeHeapPayload();
        
        type = other.type;
        priority = other.priority;
        sourceId = other.sourceId;
        targetId = other.targetId;
        timestamp = other.timestamp;
        payloadSize = other.payloadSize;
        heapPayload = other.heapPayload;
        std::memcpy(inlinePayload, other.inlinePayload, MaxInlinePayload);
        
        other.heapPayload = nullptr;
        other.payloadSize = 0;
    }
    return *this;
}

void Message::allocateHeapPayload(size_t size) {
    freeHeapPayload();
    heapPayload = std::malloc(size);
}

void Message::freeHeapPayload() {
    if (heapPayload) {
        std::free(heapPayload);
        heapPayload = nullptr;
    }
}

void Message::copyFrom(const Message& other) {
    type = other.type;
    priority = other.priority;
    sourceId = other.sourceId;
    targetId = other.targetId;
    timestamp = other.timestamp;
    payloadSize = other.payloadSize;
    std::memcpy(inlinePayload, other.inlinePayload, MaxInlinePayload);
    
    if (other.heapPayload && other.payloadSize > MaxInlinePayload) {
        allocateHeapPayload(other.payloadSize);
        std::memcpy(heapPayload, other.heapPayload, other.payloadSize);
    } else {
        heapPayload = nullptr;
    }
}

// AsyncDispatcher implementation

struct AsyncDispatcher::Impl {
    // Priority queues for different priority levels
    MPSCQueue<Message, 4096> highPriorityQueue;
    MPSCQueue<Message, 8192> normalPriorityQueue;
    MPSCQueue<Message, 2048> lowPriorityQueue;
    
    // Handlers
    std::mutex handlerMutex;
    std::unordered_map<MessageType, Handler> handlers;
    
    // Statistics
    std::atomic<uint64_t> messagesPosted{0};
    std::atomic<uint64_t> messagesProcessed{0};
    std::atomic<uint64_t> messagesDropped{0};
    std::atomic<uint64_t> queueOverflows{0};
    
    bool tryPush(Message msg) {
        bool success = false;
        
        switch (msg.priority) {
            case Priority::RealTime:
            case Priority::High:
                success = highPriorityQueue.tryPush(std::move(msg));
                break;
            case Priority::Normal:
                success = normalPriorityQueue.tryPush(std::move(msg));
                break;
            case Priority::Low:
                success = lowPriorityQueue.tryPush(std::move(msg));
                break;
        }
        
        return success;
    }
    
    std::optional<Message> tryPop() {
        // Process high priority first
        auto msg = highPriorityQueue.tryPop();
        if (msg) return msg;
        
        msg = normalPriorityQueue.tryPop();
        if (msg) return msg;
        
        return lowPriorityQueue.tryPop();
    }
    
    bool isEmpty() const {
        return highPriorityQueue.empty() && 
               normalPriorityQueue.empty() && 
               lowPriorityQueue.empty();
    }
};

AsyncDispatcher::AsyncDispatcher() : impl_(std::make_unique<Impl>()) {}

AsyncDispatcher::~AsyncDispatcher() = default;

bool AsyncDispatcher::post(Message msg) {
    // Set timestamp if not set
    if (msg.timestamp == 0) {
        msg.timestamp = static_cast<uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
    }
    
    bool success = impl_->tryPush(std::move(msg));
    
    if (success) {
        impl_->messagesPosted.fetch_add(1, std::memory_order_relaxed);
    } else {
        impl_->queueOverflows.fetch_add(1, std::memory_order_relaxed);
        impl_->messagesDropped.fetch_add(1, std::memory_order_relaxed);
    }
    
    return success;
}

size_t AsyncDispatcher::process(size_t maxMessages) {
    size_t processed = 0;
    
    while (maxMessages == 0 || processed < maxMessages) {
        auto msg = impl_->tryPop();
        if (!msg) {
            break;
        }
        
        // Find and call handler
        Handler handler;
        {
            std::lock_guard lock(impl_->handlerMutex);
            auto it = impl_->handlers.find(msg->type);
            if (it != impl_->handlers.end()) {
                handler = it->second;
            }
        }
        
        if (handler) {
            handler(*msg);
        }
        
        impl_->messagesProcessed.fetch_add(1, std::memory_order_relaxed);
        ++processed;
    }
    
    return processed;
}

void AsyncDispatcher::registerHandler(MessageType type, Handler handler) {
    std::lock_guard lock(impl_->handlerMutex);
    impl_->handlers[type] = std::move(handler);
}

void AsyncDispatcher::unregisterHandler(MessageType type) {
    std::lock_guard lock(impl_->handlerMutex);
    impl_->handlers.erase(type);
}

size_t AsyncDispatcher::pendingCount() const {
    return impl_->highPriorityQueue.size() + 
           impl_->normalPriorityQueue.size() + 
           impl_->lowPriorityQueue.size();
}

bool AsyncDispatcher::hasPending() const {
    return !impl_->isEmpty();
}

AsyncDispatcher::Stats AsyncDispatcher::getStats() const {
    Stats stats;
    stats.messagesPosted = impl_->messagesPosted.load(std::memory_order_relaxed);
    stats.messagesProcessed = impl_->messagesProcessed.load(std::memory_order_relaxed);
    stats.messagesDropped = impl_->messagesDropped.load(std::memory_order_relaxed);
    stats.queueOverflows = impl_->queueOverflows.load(std::memory_order_relaxed);
    return stats;
}

void AsyncDispatcher::resetStats() {
    impl_->messagesPosted.store(0, std::memory_order_relaxed);
    impl_->messagesProcessed.store(0, std::memory_order_relaxed);
    impl_->messagesDropped.store(0, std::memory_order_relaxed);
    impl_->queueOverflows.store(0, std::memory_order_relaxed);
}

}  // namespace cppmusic::engine::concurrency
