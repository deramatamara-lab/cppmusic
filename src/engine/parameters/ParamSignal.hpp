#pragma once
/**
 * @file ParamSignal.hpp
 * @brief Reactive signal object for parameter values with observer notification.
 */

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace cppmusic::engine::parameters {

/**
 * @brief Unique identifier for parameters.
 */
using ParamId = std::uint32_t;

/**
 * @brief Invalid parameter ID sentinel value.
 */
constexpr ParamId InvalidParamId = 0;

/**
 * @brief Observer interface for parameter changes.
 */
class ParamObserver {
public:
    virtual ~ParamObserver() = default;
    
    /**
     * @brief Called when parameter value changes.
     * @param paramId The parameter that changed.
     * @param newValue The new value.
     */
    virtual void onParamChanged(ParamId paramId, float newValue) = 0;
};

/**
 * @brief Specification for creating a parameter.
 */
struct ParamSpec {
    std::string name;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
    bool isAutomatable = true;
};

/**
 * @brief A reactive signal object that notifies observers when changed.
 * 
 * Thread-safe for value access from audio thread.
 * Observer management should be done from non-audio thread.
 */
class ParamSignal {
public:
    /**
     * @brief Construct a parameter signal with given specification.
     */
    explicit ParamSignal(ParamId id, const ParamSpec& spec);
    
    ~ParamSignal() = default;
    
    // Non-copyable, non-movable
    ParamSignal(const ParamSignal&) = delete;
    ParamSignal& operator=(const ParamSignal&) = delete;
    ParamSignal(ParamSignal&&) = delete;
    ParamSignal& operator=(ParamSignal&&) = delete;
    
    // =========================================================================
    // Identification
    // =========================================================================
    
    /**
     * @brief Get the parameter ID.
     */
    [[nodiscard]] ParamId getId() const noexcept { return id_; }
    
    /**
     * @brief Get the parameter name.
     */
    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
    
    // =========================================================================
    // Value Access (thread-safe for audio thread reads)
    // =========================================================================
    
    /**
     * @brief Get the current value (thread-safe).
     */
    [[nodiscard]] float getValue() const noexcept {
        return value_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Set the value (notifies observers).
     */
    void setValue(float value);
    
    /**
     * @brief Set value from normalized 0-1 range.
     */
    void setValueNormalized(float normalized);
    
    /**
     * @brief Get the value normalized to 0-1 range.
     */
    [[nodiscard]] float getValueNormalized() const noexcept;
    
    // =========================================================================
    // Range and Metadata
    // =========================================================================
    
    [[nodiscard]] float getMinValue() const noexcept { return minValue_; }
    [[nodiscard]] float getMaxValue() const noexcept { return maxValue_; }
    [[nodiscard]] float getDefaultValue() const noexcept { return defaultValue_; }
    [[nodiscard]] bool isAutomatable() const noexcept { return isAutomatable_; }
    
    // =========================================================================
    // Modulation
    // =========================================================================
    
    /**
     * @brief Get the current modulated value (base + modulation).
     */
    [[nodiscard]] float getModulatedValue() const noexcept;
    
    /**
     * @brief Set the modulation offset (applied on top of base value).
     * @param amount Modulation amount (can be negative).
     */
    void setModulationAmount(float amount) noexcept;
    
    /**
     * @brief Get the current modulation amount.
     */
    [[nodiscard]] float getModulationAmount() const noexcept {
        return modAmount_.load(std::memory_order_acquire);
    }
    
    // =========================================================================
    // Observer Management (call from non-audio thread)
    // =========================================================================
    
    /**
     * @brief Add an observer for value changes.
     */
    void addObserver(ParamObserver* observer);
    
    /**
     * @brief Remove an observer.
     */
    void removeObserver(ParamObserver* observer);
    
private:
    /**
     * @brief Clamp value to valid range.
     */
    [[nodiscard]] float clampValue(float value) const noexcept;
    
    /**
     * @brief Notify all observers of value change.
     */
    void notifyObservers(float newValue);
    
    ParamId id_;
    std::string name_;
    float minValue_;
    float maxValue_;
    float defaultValue_;
    bool isAutomatable_;
    
    std::atomic<float> value_;
    std::atomic<float> modAmount_{0.0f};
    
    std::vector<ParamObserver*> observers_;
    std::mutex observerMutex_;
};

} // namespace cppmusic::engine::parameters
