#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <unordered_map>
#include <functional>
#include <deque>
#include <memory>
#include <chrono>

namespace daw::ui::animation
{

/**
 * ProfessionalAnimationEngine - Advanced Animation System for Professional UI
 *
 * Features:
 * - Frame-rate independent animations
 * - Advanced easing functions
 * - Audio-reactive animations
 * - Performance optimization
 * - Animation orchestration
 * - Timeline-based animations
 * - Keyframe interpolation
 * - Animation blending
 */
class ProfessionalAnimationEngine
{
public:
    //==============================================================================
    // Animation Types
    enum class AnimationType
    {
        Position,
        Scale,
        Rotation,
        Opacity,
        Color,
        Size,
        Custom,
        AudioReactive,
        Physics,
        Spring
    };

    //==============================================================================
    // Easing Functions
    enum class EasingType
    {
        Linear,
        EaseIn,
        EaseOut,
        EaseInOut,
        EaseOutIn,
        Bounce,
        Elastic,
        Back,
        Sine,
        Quad,
        Cubic,
        Quart,
        Quint,
        Expo,
        Circ,
        Custom
    };

    //==============================================================================
    // Animation State
    enum class AnimationState
    {
        Idle,
        Playing,
        Paused,
        Stopped,
        Completed,
        Error
    };

    //==============================================================================
    // Animation Configuration
    struct AnimationConfig
    {
        AnimationType type = AnimationType::Position;
        EasingType easing = EasingType::EaseInOut;
        float duration = 1.0f;
        float delay = 0.0f;
        bool autoReverse = false;
        int repeatCount = 1;
        bool infinite = false;
        float speed = 1.0f;
        bool audioReactive = false;
        float audioSensitivity = 1.0f;
        bool physicsEnabled = false;
        float springStiffness = 100.0f;
        float springDamping = 10.0f;
        float mass = 1.0f;
        bool useKeyframes = false;
        std::vector<float> keyframeTimes;
        std::vector<float> keyframeValues;
    };

    //==============================================================================
    // Animation Target
    struct AnimationTarget
    {
        juce::Component* component = nullptr;
        juce::String propertyName;
        float startValue = 0.0f;
        float endValue = 1.0f;
        juce::Point<float> startPosition;
        juce::Point<float> endPosition;
        juce::Rectangle<float> startBounds;
        juce::Rectangle<float> endBounds;
        float startScale = 1.0f;
        float endScale = 1.0f;
        float startRotation = 0.0f;
        float endRotation = 360.0f;
        float startOpacity = 1.0f;
        float endOpacity = 0.0f;
        juce::Colour startColor;
        juce::Colour endColor;
        std::function<void(float)> customCallback;
    };

    //==============================================================================
    // Animation Instance
    struct AnimationInstance
    {
        juce::String id;
        AnimationConfig config;
        AnimationTarget target;
        AnimationState state = AnimationState::Idle;
        float currentTime = 0.0f;
        float currentValue = 0.0f;
        int currentRepeat = 0;
        bool isReversed = false;
        juce::int64 startTime = 0;
        juce::int64 lastUpdateTime = 0;
        float audioLevel = 0.0f;
        std::function<void(const AnimationInstance&)> onComplete;
        std::function<void(const AnimationInstance&)> onUpdate;
        std::function<void(const AnimationInstance&)> onError;
    };

    //==============================================================================
    // Performance Metrics
    struct PerformanceMetrics
    {
        float frameRate = 60.0f;
        int activeAnimations = 0;
        float cpuUsage = 0.0f;
        float memoryUsage = 0.0f;
        int drawCalls = 0;
        float averageAnimationTime = 0.0f;
        int completedAnimations = 0;
        int failedAnimations = 0;
    };

    //==============================================================================
    // Audio Analysis
    struct AudioAnalysis
    {
        float level = 0.0f;
        float frequency = 0.0f;
        float spectrum[64];
        float bass = 0.0f;
        float mid = 0.0f;
        float treble = 0.0f;
        float peak = 0.0f;
        float rms = 0.0f;
    };

    //==============================================================================
    // Animation Event
    struct AnimationEvent
    {
        enum class EventType
        {
            Started,
            Updated,
            Completed,
            Paused,
            Resumed,
            Stopped,
            Error,
            AudioReactive,
            PhysicsUpdate
        };

        EventType type;
        juce::String animationId;
        float value = 0.0f;
        float time = 0.0f;
        juce::int64 timestamp = 0;
    };

    //==============================================================================
    // Animation Callback
    using AnimationCallback = std::function<void(const AnimationEvent&)>;

    //==============================================================================
    // Main Animation Engine
    ProfessionalAnimationEngine();
    ~ProfessionalAnimationEngine();

    // Lifecycle
    void initialize();
    void shutdown();
    void update();

    // Animation management
    juce::String createAnimation(const AnimationConfig& config, const AnimationTarget& target);
    void startAnimation(const juce::String& animationId);
    void pauseAnimation(const juce::String& animationId);
    void resumeAnimation(const juce::String& animationId);
    void stopAnimation(const juce::String& animationId);
    void removeAnimation(const juce::String& animationId);

    // Animation queries
    AnimationState getAnimationState(const juce::String& animationId) const;
    float getAnimationProgress(const juce::String& animationId) const;
    float getAnimationValue(const juce::String& animationId) const;
    bool isAnimationActive(const juce::String& animationId) const;
    bool isAnimationCompleted(const juce::String& animationId) const;

    // Animation control
    void setAnimationSpeed(const juce::String& animationId, float speed);
    void setAnimationDuration(const juce::String& animationId, float duration);
    void setAnimationEasing(const juce::String& animationId, EasingType easing);

    // Audio reactivity
    void setAudioReactive(const juce::String& animationId, bool enabled);
    void setAudioSensitivity(const juce::String& animationId, float sensitivity);
    void updateAudioAnalysis(const AudioAnalysis& analysis);
    float getAudioLevel() const;

    // Physics
    void setPhysicsEnabled(const juce::String& animationId, bool enabled);
    void setSpringStiffness(const juce::String& animationId, float stiffness);
    void setSpringDamping(const juce::String& animationId, float damping);
    void setMass(const juce::String& animationId, float mass);

    // Easing functions
    float applyEasing(float t, EasingType easing) const;

    // Performance
    PerformanceMetrics getPerformanceMetrics() const;
    void setTargetFrameRate(float frameRate);
    void setPerformanceMode(bool enabled);
    void optimizePerformance();
    void clearCompletedAnimations();

    // Event listeners
    void addAnimationListener(AnimationCallback callback);
    void removeAnimationListener(AnimationCallback callback);

    // Utility functions
    juce::String generateAnimationId() const;
    void resetAllAnimations();
    void pauseAllAnimations();
    void resumeAllAnimations();
    void stopAllAnimations();

private:
    //==============================================================================
    // Implementation
    struct Impl;
    std::unique_ptr<Impl> pImpl;

    // Internal methods
    void updateAnimation(AnimationInstance& animation, float deltaTime);
    void applyAnimationValue(const AnimationInstance& animation, float value);
    void notifyAnimationEvent(const AnimationEvent& event);
    void cleanupCompletedAnimations();
    float calculateEasingValue(float t, EasingType easing) const;
    float interpolateKeyframes(float time, const std::vector<float>& times, const std::vector<float>& values) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProfessionalAnimationEngine)
};

} // namespace daw::ui::animation

