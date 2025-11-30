#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace daw::ui::core
{

/**
 * Physics-based animation system using spring dynamics.
 * Provides natural, responsive animations that feel organic.
 */
struct PhysicsAnimation
{
    float value = 0.0f;
    float velocity = 0.0f;
    float target = 0.0f;
    float stiffness = 180.0f;   // Spring stiffness (higher = faster)
    float damping = 12.0f;       // Damping ratio (higher = less oscillation)
    float mass = 1.0f;           // Mass (higher = slower)
    float precision = 0.001f;    // Precision threshold for stopping

    PhysicsAnimation() = default;

    PhysicsAnimation(float initialValue, float springStiffness = 180.0f, float dampingRatio = 12.0f)
        : value(initialValue), target(initialValue), stiffness(springStiffness), damping(dampingRatio) {}

    /**
     * Update the animation with delta time.
     * Returns true if still animating, false if at rest.
     */
    bool update(float deltaTime)
    {
        if (isAtRest())
            return false;

        // Spring physics calculation
        float force = (target - value) * stiffness;
        float dampingForce = velocity * damping;
        float acceleration = (force - dampingForce) / mass;

        velocity += acceleration * deltaTime;
        value += velocity * deltaTime;

        // Check if we've reached equilibrium
        if (std::abs(value - target) < precision && std::abs(velocity) < precision)
        {
            value = target;
            velocity = 0.0f;
            return false;
        }

        return true;
    }

    /** Set a new target with optional velocity preservation */
    void setTarget(float newTarget, bool preserveVelocity = false)
    {
        target = newTarget;
        if (!preserveVelocity)
            velocity *= 0.8f; // Dampen existing velocity for smoother transitions
    }

    /** Instantly snap to target */
    void snapToTarget()
    {
        value = target;
        velocity = 0.0f;
    }

    /** Check if animation is at rest */
    bool isAtRest() const
    {
        return std::abs(value - target) < precision && std::abs(velocity) < precision;
    }

    /** Get normalized progress (0.0 to 1.0) */
    float getProgress() const
    {
        return juce::jlimit(0.0f, 1.0f, value);
    }

    /** Configure spring parameters for different feels */
    void setSpringParams(float stiff, float damp, float m = 1.0f)
    {
        stiffness = stiff;
        damping = damp;
        mass = m;
    }

    // Preset configurations
    static PhysicsAnimation bouncy(float initialValue = 0.0f)
    {
        return PhysicsAnimation(initialValue, 220.0f, 8.0f);
    }

    static PhysicsAnimation smooth(float initialValue = 0.0f)
    {
        return PhysicsAnimation(initialValue, 150.0f, 15.0f);
    }

    static PhysicsAnimation snappy(float initialValue = 0.0f)
    {
        return PhysicsAnimation(initialValue, 300.0f, 20.0f);
    }

    static PhysicsAnimation gentle(float initialValue = 0.0f)
    {
        return PhysicsAnimation(initialValue, 100.0f, 12.0f);
    }
};

/**
 * Ultra-sleek animation state for sophisticated UI components.
 * Complete animation state management for all interaction types.
 */
struct UltraSleekAnimationState
{
    // Core interaction states
    PhysicsAnimation hoverProgress = PhysicsAnimation::smooth();
    PhysicsAnimation focusProgress = PhysicsAnimation::bouncy();
    PhysicsAnimation pressProgress = PhysicsAnimation::snappy();
    PhysicsAnimation activeProgress = PhysicsAnimation::smooth();

    // Visual effect states
    PhysicsAnimation glowIntensity = PhysicsAnimation::gentle();
    PhysicsAnimation scaleProgress = PhysicsAnimation::smooth(1.0f);
    PhysicsAnimation alphaProgress = PhysicsAnimation::smooth(1.0f);
    PhysicsAnimation rotationAngle = PhysicsAnimation::smooth();

    // Advanced effect states
    PhysicsAnimation blurRadius = PhysicsAnimation::gentle();
    PhysicsAnimation saturation = PhysicsAnimation::gentle(1.0f);
    PhysicsAnimation brightness = PhysicsAnimation::gentle(1.0f);

    // Ripple effect states
    juce::Point<float> rippleCenter{0, 0};
    PhysicsAnimation rippleProgress = PhysicsAnimation::bouncy();
    PhysicsAnimation rippleAlpha = PhysicsAnimation::smooth();
    PhysicsAnimation rippleScale = PhysicsAnimation::bouncy(1.0f);

    // Audio-reactive states
    PhysicsAnimation audioReactivity = PhysicsAnimation::snappy();
    PhysicsAnimation spectrumIntensity = PhysicsAnimation::bouncy();

    // Animation progress for time-based effects (0.0 to 1.0, wraps)
    float progress = 0.0f;

    bool isAnimating = false;

    /**
     * Update all animations and return true if any are still active.
     */
    bool updateAll(float deltaTime)
    {
        bool stillAnimating = false;

        stillAnimating |= hoverProgress.update(deltaTime);
        stillAnimating |= focusProgress.update(deltaTime);
        stillAnimating |= pressProgress.update(deltaTime);
        stillAnimating |= activeProgress.update(deltaTime);

        stillAnimating |= glowIntensity.update(deltaTime);
        stillAnimating |= scaleProgress.update(deltaTime);
        stillAnimating |= alphaProgress.update(deltaTime);
        stillAnimating |= rotationAngle.update(deltaTime);

        stillAnimating |= blurRadius.update(deltaTime);
        stillAnimating |= saturation.update(deltaTime);
        stillAnimating |= brightness.update(deltaTime);

        stillAnimating |= rippleProgress.update(deltaTime);
        stillAnimating |= rippleAlpha.update(deltaTime);
        stillAnimating |= rippleScale.update(deltaTime);

        stillAnimating |= audioReactivity.update(deltaTime);
        stillAnimating |= spectrumIntensity.update(deltaTime);

        isAnimating = stillAnimating;
        return stillAnimating;
    }

    /** Reset all animations to default state */
    void reset()
    {
        hoverProgress.snapToTarget();
        focusProgress.snapToTarget();
        pressProgress.snapToTarget();
        activeProgress.snapToTarget();

        glowIntensity.setTarget(0.0f);
        scaleProgress.setTarget(1.0f);
        alphaProgress.setTarget(1.0f);
        rotationAngle.setTarget(0.0f);

        blurRadius.setTarget(0.0f);
        saturation.setTarget(1.0f);
        brightness.setTarget(1.0f);

        rippleProgress.setTarget(0.0f);
        rippleAlpha.setTarget(0.0f);
        rippleScale.setTarget(1.0f);

        audioReactivity.setTarget(0.0f);
        spectrumIntensity.setTarget(0.0f);

        progress = 0.0f;
    }
};

} // namespace daw::ui::core

