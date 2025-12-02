/**
 * @file Animation.hpp
 * @brief Animation utilities for smooth UI transitions and micro-interactions.
 *
 * Provides lightweight animation helpers for polished UI feel:
 * - Easing functions for natural motion
 * - AnimatedValue class for smooth property transitions
 * - Performance-optimized with no per-frame allocations
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <functional>

namespace cppmusic::ui::animation {

/**
 * @brief Easing function type
 */
using EasingFunction = std::function<float(float)>;

/**
 * @brief Standard easing functions for animations
 */
namespace Easing {
    inline float linear(float t) { return t; }
    
    inline float easeInQuad(float t) { return t * t; }
    inline float easeOutQuad(float t) { return t * (2.0f - t); }
    inline float easeInOutQuad(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }
    
    inline float easeInCubic(float t) { return t * t * t; }
    inline float easeOutCubic(float t) {
        const float f = t - 1.0f;
        return f * f * f + 1.0f;
    }
    inline float easeInOutCubic(float t) {
        return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
    }
    
    inline float easeInQuart(float t) { return t * t * t * t; }
    inline float easeOutQuart(float t) {
        const float f = t - 1.0f;
        return 1.0f - f * f * f * f;
    }
    inline float easeInOutQuart(float t) {
        return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - 8.0f * (t - 1.0f) * (t - 1.0f) * (t - 1.0f) * (t - 1.0f);
    }
    
    // Elastic easing for bouncy effects
    inline float easeOutElastic(float t) {
        if (t == 0.0f || t == 1.0f) return t;
        const float p = 0.3f;
        return std::pow(2.0f, -10.0f * t) * std::sin((t - p / 4.0f) * (2.0f * juce::MathConstants<float>::pi) / p) + 1.0f;
    }
    
    // Back easing for slight overshoot
    inline float easeOutBack(float t) {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;
        return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
    }
}

/**
 * @brief Animated value that smoothly transitions between values
 * 
 * Usage:
 *   AnimatedValue<float> opacity;
 *   opacity.setTarget(1.0f, 200); // Animate to 1.0 over 200ms
 *   opacity.update(16); // Call in timer callback (16ms = 60fps)
 *   float current = opacity.getValue();
 */
template<typename T>
class AnimatedValue {
public:
    AnimatedValue(T initialValue = T{}) 
        : current_(initialValue)
        , target_(initialValue)
        , duration_(0.0f)
        , elapsed_(0.0f)
        , easing_(Easing::easeOutCubic)
    {}
    
    /**
     * @brief Set a new target value with animation
     * @param target Target value
     * @param durationMs Animation duration in milliseconds
     * @param easingFunc Easing function to use (default: easeOutCubic)
     */
    void setTarget(T target, float durationMs, EasingFunction easingFunc = Easing::easeOutCubic) {
        if (target == target_ && elapsed_ >= duration_) {
            return; // Already at target
        }
        
        start_ = current_;
        target_ = target;
        duration_ = durationMs;
        elapsed_ = 0.0f;
        easing_ = easingFunc;
    }
    
    /**
     * @brief Set value immediately without animation
     */
    void setValue(T value) {
        current_ = value;
        target_ = value;
        start_ = value;
        elapsed_ = duration_;
    }
    
    /**
     * @brief Update animation state
     * @param deltaMs Time elapsed since last update in milliseconds
     * @return true if animation is still in progress
     */
    bool update(float deltaMs) {
        if (elapsed_ >= duration_) {
            current_ = target_;
            return false;
        }
        
        elapsed_ += deltaMs;
        if (elapsed_ >= duration_) {
            current_ = target_;
            return false;
        }
        
        const float t = elapsed_ / duration_;
        const float easedT = easing_(t);
        current_ = interpolate(start_, target_, easedT);
        return true;
    }
    
    /**
     * @brief Get current value
     */
    T getValue() const { return current_; }
    
    /**
     * @brief Get target value
     */
    T getTarget() const { return target_; }
    
    /**
     * @brief Check if animation is in progress
     */
    bool isAnimating() const { return elapsed_ < duration_; }
    
private:
    T current_;
    T target_;
    T start_;
    float duration_;
    float elapsed_;
    EasingFunction easing_;
    
    // Interpolation helpers
    static T interpolate(const T& a, const T& b, float t) {
        if constexpr (std::is_arithmetic_v<T>) {
            return static_cast<T>(a + (b - a) * t);
        } else if constexpr (std::is_same_v<T, juce::Colour>) {
            return a.interpolatedWith(b, t);
        } else if constexpr (std::is_same_v<T, juce::Point<float>>) {
            return a + (b - a) * t;
        } else {
            // For other types, assume they support operator+ and operator*
            return a + (b - a) * t;
        }
    }
};

/**
 * @brief Animation controller for managing multiple animated values
 * 
 * Implements juce::Timer to provide centralized animation updates
 */
class AnimationController : public juce::Timer {
public:
    AnimationController() : lastTime_(juce::Time::getMillisecondCounterHiRes()) {
        callbacks_.ensureStorageAllocated(16); // Pre-allocate to avoid reallocation
        startTimerHz(60); // 60 FPS
    }
    
    ~AnimationController() override {
        stopTimer();
    }
    
    /**
     * @brief Register a callback for animation updates
     */
    void addUpdateCallback(std::function<void(float)> callback) {
        callbacks_.add(std::move(callback));
    }
    
    /**
     * @brief Clear all callbacks
     */
    void clearCallbacks() {
        callbacks_.clearQuick();
    }
    
private:
    void timerCallback() override {
        const auto now = juce::Time::getMillisecondCounterHiRes();
        const float deltaMs = static_cast<float>(now - lastTime_);
        lastTime_ = now;
        
        for (auto& callback : callbacks_) {
            callback(deltaMs);
        }
    }
    
    double lastTime_;
    juce::Array<std::function<void(float)>> callbacks_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnimationController)
};

/**
 * @brief Component mixin that provides animation support
 * 
 * Usage:
 *   class MyComponent : public juce::Component, public AnimatedComponent {
 *       void someMethod() {
 *           startAnimation([this](float delta) {
 *               opacity_.update(delta);
 *               repaint();
 *               return opacity_.isAnimating();
 *           });
 *       }
 *   };
 */
class AnimatedComponent : private juce::Timer {
public:
    AnimatedComponent() = default;
    virtual ~AnimatedComponent() override { stopTimer(); }
    
protected:
    /**
     * @brief Start an animation
     * @param updateFunc Update function called each frame. Return false to stop.
     */
    void startAnimation(std::function<bool(float)> updateFunc) {
        updateFunc_ = std::move(updateFunc);
        lastTime_ = juce::Time::getMillisecondCounterHiRes();
        startTimerHz(60); // 60 FPS
    }
    
    /**
     * @brief Stop the current animation
     */
    void stopAnimation() {
        stopTimer();
        updateFunc_ = nullptr;
    }
    
private:
    void timerCallback() override {
        if (!updateFunc_) {
            stopTimer();
            return;
        }
        
        const auto now = juce::Time::getMillisecondCounterHiRes();
        const float deltaMs = static_cast<float>(now - lastTime_);
        lastTime_ = now;
        
        const bool shouldContinue = updateFunc_(deltaMs);
        if (!shouldContinue) {
            stopTimer();
        }
    }
    
    std::function<bool(float)> updateFunc_;
    double lastTime_{0.0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnimatedComponent)
};

} // namespace cppmusic::ui::animation
