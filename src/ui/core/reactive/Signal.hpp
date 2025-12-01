/**
 * @file Signal.hpp
 * @brief Reactive data binding with frame-coalesced flushing
 * 
 * Implements a Signal<T> system for reactive UI updates with:
 * - Frame-coalesced flushing to batch updates
 * - Lock-free queue bridging from engine thread to UI aggregator
 * - Subscription management with automatic cleanup
 */
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace daw::ui::reactive
{

// Forward declarations
class SignalAggregator;

/**
 * @brief Subscription handle for automatic cleanup
 */
class Subscription
{
public:
    Subscription() = default;
    Subscription(std::function<void()> unsub) : unsubscribe_(std::move(unsub)), active_(true) {}
    ~Subscription() { if (active_) unsubscribe(); }

    // Move only
    Subscription(Subscription&& other) noexcept
        : unsubscribe_(std::move(other.unsubscribe_)), active_(other.active_)
    {
        other.active_ = false;
    }
    Subscription& operator=(Subscription&& other) noexcept
    {
        if (this != &other) {
            if (active_) unsubscribe();
            unsubscribe_ = std::move(other.unsubscribe_);
            active_ = other.active_;
            other.active_ = false;
        }
        return *this;
    }
    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

    void unsubscribe()
    {
        if (active_ && unsubscribe_) {
            unsubscribe_();
            active_ = false;
        }
    }

    [[nodiscard]] bool isActive() const { return active_; }

private:
    std::function<void()> unsubscribe_;
    bool active_{false};
};

/**
 * @brief Base class for type-erased signal operations
 */
class SignalBase
{
public:
    virtual ~SignalBase() = default;
    virtual void flush() = 0;
    virtual bool isDirty() const = 0;
    [[nodiscard]] virtual std::size_t getSubscriberCount() const = 0;
};

/**
 * @brief Reactive signal with frame-coalesced flushing
 * 
 * Updates are collected and only propagated to subscribers during flush(),
 * which should be called once per frame by the SignalAggregator.
 * 
 * @tparam T The value type held by the signal
 */
template<typename T>
class Signal : public SignalBase
{
public:
    using ValueType = T;
    using Callback = std::function<void(const T&)>;

    explicit Signal(T initial = T{})
        : value_(std::move(initial)), pendingValue_(value_), dirty_(false)
    {}

    ~Signal() override = default;

    // Non-copyable, non-movable (subscribers hold references)
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;
    Signal(Signal&&) = delete;
    Signal& operator=(Signal&&) = delete;

    /**
     * @brief Get current value (read-only)
     */
    [[nodiscard]] const T& get() const { return value_; }

    /**
     * @brief Set new value (marks dirty, deferred until flush)
     * Thread-safe: can be called from any thread
     */
    void set(const T& newValue)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingValue_ = newValue;
        dirty_.store(true, std::memory_order_release);
    }

    /**
     * @brief Set new value (move version)
     */
    void set(T&& newValue)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingValue_ = std::move(newValue);
        dirty_.store(true, std::memory_order_release);
    }

    /**
     * @brief Update value using a modifier function
     */
    template<typename F>
    void update(F&& modifier)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        modifier(pendingValue_);
        dirty_.store(true, std::memory_order_release);
    }

    /**
     * @brief Subscribe to value changes
     * @param callback Called with new value after flush
     * @return Subscription handle (unsubscribes on destruction)
     */
    [[nodiscard]] Subscription subscribe(Callback callback)
    {
        std::lock_guard<std::mutex> lock(subscriberMutex_);
        auto id = nextId_++;
        subscribers_.push_back({id, std::move(callback)});
        
        return Subscription([this, id]() {
            std::lock_guard<std::mutex> lock(subscriberMutex_);
            subscribers_.erase(
                std::remove_if(subscribers_.begin(), subscribers_.end(),
                    [id](const auto& sub) { return sub.id == id; }),
                subscribers_.end());
        });
    }

    /**
     * @brief Flush pending value to subscribers
     * Should be called once per frame by SignalAggregator
     */
    void flush() override
    {
        if (!dirty_.load(std::memory_order_acquire)) return;

        T newValue;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            newValue = std::move(pendingValue_);
            pendingValue_ = newValue;  // Keep copy for next update
            dirty_.store(false, std::memory_order_release);
        }

        // Only notify if value changed
        if (!(newValue == value_)) {
            value_ = std::move(newValue);
            notifySubscribers();
        }
    }

    /**
     * @brief Check if signal has pending updates
     */
    [[nodiscard]] bool isDirty() const override
    {
        return dirty_.load(std::memory_order_acquire);
    }

    /**
     * @brief Get number of active subscribers
     */
    [[nodiscard]] std::size_t getSubscriberCount() const override
    {
        std::lock_guard<std::mutex> lock(subscriberMutex_);
        return subscribers_.size();
    }

private:
    void notifySubscribers()
    {
        std::lock_guard<std::mutex> lock(subscriberMutex_);
        for (const auto& sub : subscribers_) {
            if (sub.callback) {
                sub.callback(value_);
            }
        }
    }

    struct SubscriberEntry
    {
        uint64_t id;
        Callback callback;
    };

    T value_;
    T pendingValue_;
    std::atomic<bool> dirty_;
    mutable std::mutex mutex_;
    
    std::vector<SubscriberEntry> subscribers_;
    uint64_t nextId_{0};
    mutable std::mutex subscriberMutex_;
};

/**
 * @brief Lock-free single-producer single-consumer queue for thread bridging
 */
template<typename T, std::size_t Capacity = 1024>
class LockFreeQueue
{
public:
    LockFreeQueue() : head_(0), tail_(0) {}

    /**
     * @brief Try to push an item (producer thread)
     * @return true if pushed, false if queue full
     */
    bool tryPush(const T& item)
    {
        const auto current_tail = tail_.load(std::memory_order_relaxed);
        const auto next_tail = (current_tail + 1) % Capacity;
        
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;  // Queue full
        }
        
        buffer_[current_tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    /**
     * @brief Try to pop an item (consumer thread)
     * @return Item if available, nullopt if empty
     */
    std::optional<T> tryPop()
    {
        const auto current_head = head_.load(std::memory_order_relaxed);
        
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return std::nullopt;  // Queue empty
        }
        
        T item = std::move(buffer_[current_head]);
        head_.store((current_head + 1) % Capacity, std::memory_order_release);
        return item;
    }

    /**
     * @brief Check if queue is empty
     */
    [[nodiscard]] bool empty() const
    {
        return head_.load(std::memory_order_acquire) == 
               tail_.load(std::memory_order_acquire);
    }

    /**
     * @brief Get approximate size
     */
    [[nodiscard]] std::size_t size() const
    {
        const auto h = head_.load(std::memory_order_acquire);
        const auto t = tail_.load(std::memory_order_acquire);
        return (t >= h) ? (t - h) : (Capacity - h + t);
    }

private:
    std::array<T, Capacity> buffer_;
    std::atomic<std::size_t> head_;
    std::atomic<std::size_t> tail_;
};

/**
 * @brief Aggregates multiple signals and flushes them in a single frame pass
 */
class SignalAggregator
{
public:
    /**
     * @brief Register a signal for aggregated flushing
     */
    void registerSignal(SignalBase* signal)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        signals_.push_back(signal);
    }

    /**
     * @brief Unregister a signal
     */
    void unregisterSignal(SignalBase* signal)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        signals_.erase(
            std::remove(signals_.begin(), signals_.end(), signal),
            signals_.end());
    }

    /**
     * @brief Flush all registered signals
     * Call once per frame from UI thread
     */
    void flush()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        flushCount_++;
        
        for (auto* signal : signals_) {
            if (signal->isDirty()) {
                signal->flush();
                dirtySignalCount_++;
            }
        }
    }

    /**
     * @brief Get total flush count
     */
    [[nodiscard]] uint64_t getFlushCount() const { return flushCount_; }

    /**
     * @brief Get count of dirty signals in last flush
     */
    [[nodiscard]] uint64_t getDirtySignalCount() const { return dirtySignalCount_; }

    /**
     * @brief Get total registered signal count
     */
    [[nodiscard]] std::size_t getSignalCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return signals_.size();
    }

private:
    std::vector<SignalBase*> signals_;
    mutable std::mutex mutex_;
    uint64_t flushCount_{0};
    uint64_t dirtySignalCount_{0};
};

/**
 * @brief Global signal aggregator instance
 */
inline SignalAggregator& getGlobalAggregator()
{
    static SignalAggregator instance;
    return instance;
}

} // namespace daw::ui::reactive
