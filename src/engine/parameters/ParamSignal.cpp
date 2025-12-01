/**
 * @file ParamSignal.cpp
 * @brief Implementation of the reactive parameter signal.
 */

#include "ParamSignal.hpp"
#include <algorithm>
#include <cmath>

namespace cppmusic::engine::parameters {

ParamSignal::ParamSignal(ParamId id, const ParamSpec& spec)
    : id_(id)
    , name_(spec.name)
    , minValue_(spec.minValue)
    , maxValue_(spec.maxValue)
    , defaultValue_(spec.defaultValue)
    , isAutomatable_(spec.isAutomatable) {
    value_.store(clampValue(spec.defaultValue), std::memory_order_release);
}

void ParamSignal::setValue(float value) {
    float clampedValue = clampValue(value);
    float oldValue = value_.exchange(clampedValue, std::memory_order_acq_rel);
    
    // Only notify if value actually changed
    if (std::abs(clampedValue - oldValue) > 1e-7f) {
        notifyObservers(clampedValue);
    }
}

void ParamSignal::setValueNormalized(float normalized) {
    float clamped = std::clamp(normalized, 0.0f, 1.0f);
    float value = minValue_ + clamped * (maxValue_ - minValue_);
    setValue(value);
}

float ParamSignal::getValueNormalized() const noexcept {
    float value = getValue();
    float range = maxValue_ - minValue_;
    if (range <= 0.0f) return 0.0f;
    return (value - minValue_) / range;
}

float ParamSignal::getModulatedValue() const noexcept {
    float base = getValue();
    float mod = modAmount_.load(std::memory_order_acquire);
    return clampValue(base + mod);
}

void ParamSignal::setModulationAmount(float amount) noexcept {
    modAmount_.store(amount, std::memory_order_release);
}

void ParamSignal::addObserver(ParamObserver* observer) {
    if (!observer) return;
    
    std::lock_guard<std::mutex> lock(observerMutex_);
    auto it = std::find(observers_.begin(), observers_.end(), observer);
    if (it == observers_.end()) {
        observers_.push_back(observer);
    }
}

void ParamSignal::removeObserver(ParamObserver* observer) {
    if (!observer) return;
    
    std::lock_guard<std::mutex> lock(observerMutex_);
    observers_.erase(
        std::remove(observers_.begin(), observers_.end(), observer),
        observers_.end());
}

float ParamSignal::clampValue(float value) const noexcept {
    return std::clamp(value, minValue_, maxValue_);
}

void ParamSignal::notifyObservers(float newValue) {
    std::lock_guard<std::mutex> lock(observerMutex_);
    for (auto* observer : observers_) {
        if (observer) {
            observer->onParamChanged(id_, newValue);
        }
    }
}

} // namespace cppmusic::engine::parameters
