#include "ProfessionalAnimationEngine.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace daw::ui::animation
{

//==============================================================================
// Implementation
struct ProfessionalAnimationEngine::Impl
{
    // Animation storage
    std::unordered_map<juce::String, AnimationInstance> animations;

    // Performance
    PerformanceMetrics performanceMetrics;
    float targetFrameRate = 60.0f;
    bool performanceMode = false;
    juce::int64 lastUpdateTime = 0;
    float frameTime = 16.67f; // 60 FPS default

    // Audio
    AudioAnalysis audioAnalysis;
    float audioLevel = 0.0f;

    // Event listeners
    std::vector<AnimationCallback> animationCallbacks;

    // Internal state
    bool isInitialized = false;
    int animationCounter = 0;

    // Physics simulation
    struct PhysicsState
    {
        float position = 0.0f;
        float velocity = 0.0f;
        float acceleration = 0.0f;
        float force = 0.0f;
        float gravity = 9.81f;
        float mass = 1.0f;
        float springStiffness = 100.0f;
        float springDamping = 10.0f;
        float targetPosition = 0.0f;
    };

    std::unordered_map<juce::String, PhysicsState> physicsStates;
};

//==============================================================================
ProfessionalAnimationEngine::ProfessionalAnimationEngine()
    : pImpl(std::make_unique<Impl>())
{
    // Initialize performance metrics
    pImpl->performanceMetrics.frameRate = 60.0f;
    pImpl->performanceMetrics.activeAnimations = 0;
    pImpl->performanceMetrics.cpuUsage = 0.0f;
    pImpl->performanceMetrics.memoryUsage = 0.0f;
    pImpl->performanceMetrics.drawCalls = 0;
    pImpl->performanceMetrics.averageAnimationTime = 0.0f;
    pImpl->performanceMetrics.completedAnimations = 0;
    pImpl->performanceMetrics.failedAnimations = 0;

    // Initialize audio analysis
    pImpl->audioAnalysis.level = 0.0f;
    pImpl->audioAnalysis.frequency = 0.0f;
    pImpl->audioAnalysis.bass = 0.0f;
    pImpl->audioAnalysis.mid = 0.0f;
    pImpl->audioAnalysis.treble = 0.0f;
    pImpl->audioAnalysis.peak = 0.0f;
    pImpl->audioAnalysis.rms = 0.0f;

    for (int i = 0; i < 64; ++i)
    {
        pImpl->audioAnalysis.spectrum[i] = 0.0f;
    }
}

ProfessionalAnimationEngine::~ProfessionalAnimationEngine()
{
    shutdown();
}

//==============================================================================
void ProfessionalAnimationEngine::initialize()
{
    if (pImpl->isInitialized)
        return;

    pImpl->lastUpdateTime = juce::Time::getHighResolutionTicks();
    pImpl->isInitialized = true;
}

void ProfessionalAnimationEngine::shutdown()
{
    if (!pImpl->isInitialized)
        return;

    stopAllAnimations();
    pImpl->animations.clear();
    pImpl->physicsStates.clear();
    pImpl->animationCallbacks.clear();
    pImpl->isInitialized = false;
}

void ProfessionalAnimationEngine::update()
{
    if (!pImpl->isInitialized)
        return;

    auto currentTime = juce::Time::getHighResolutionTicks();
    // Compute delta time in seconds with floating precision
    double dtSec = static_cast<double>(currentTime - pImpl->lastUpdateTime) / static_cast<double>(juce::Time::getHighResolutionTicksPerSecond());
    float deltaTime = static_cast<float>(dtSec);
    pImpl->lastUpdateTime = currentTime;

    // Limit delta time to prevent large jumps
    deltaTime = juce::jlimit(0.0f, 0.1f, deltaTime);

    // Update frame time
    pImpl->frameTime = deltaTime * 1000.0f;
    pImpl->performanceMetrics.frameRate = 1000.0f / pImpl->frameTime;

    // Update all animations
    int activeCount = 0;
    for (auto& [id, animation] : pImpl->animations)
    {
        if (animation.state == AnimationState::Playing)
        {
            updateAnimation(animation, deltaTime);
            activeCount++;
        }
    }

    pImpl->performanceMetrics.activeAnimations = activeCount;

    // Cleanup completed animations
    cleanupCompletedAnimations();
}

//==============================================================================
juce::String ProfessionalAnimationEngine::createAnimation(const AnimationConfig& config, const AnimationTarget& target)
{
    AnimationInstance instance;
    instance.id = generateAnimationId();
    instance.config = config;
    instance.target = target;
    instance.state = AnimationState::Idle;
    instance.currentTime = 0.0f;
    instance.currentValue = target.startValue;
    instance.startTime = juce::Time::getHighResolutionTicks();
    instance.lastUpdateTime = instance.startTime;

    pImpl->animations[instance.id] = instance;

    return instance.id;
}

void ProfessionalAnimationEngine::startAnimation(const juce::String& animationId)
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end())
    {
        it->second.state = AnimationState::Playing;
        it->second.startTime = juce::Time::getHighResolutionTicks();
        it->second.lastUpdateTime = it->second.startTime;

        AnimationEvent event;
        event.type = AnimationEvent::EventType::Started;
        event.animationId = animationId;
        event.timestamp = it->second.startTime;
        notifyAnimationEvent(event);
    }
}

void ProfessionalAnimationEngine::pauseAnimation(const juce::String& animationId)
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end() && it->second.state == AnimationState::Playing)
    {
        it->second.state = AnimationState::Paused;

        AnimationEvent event;
        event.type = AnimationEvent::EventType::Paused;
        event.animationId = animationId;
        event.timestamp = juce::Time::getHighResolutionTicks();
        notifyAnimationEvent(event);
    }
}

void ProfessionalAnimationEngine::resumeAnimation(const juce::String& animationId)
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end() && it->second.state == AnimationState::Paused)
    {
        it->second.state = AnimationState::Playing;
        it->second.startTime = juce::Time::getHighResolutionTicks() -
                               static_cast<juce::int64>(it->second.currentTime * juce::Time::getHighResolutionTicksPerSecond());

        AnimationEvent event;
        event.type = AnimationEvent::EventType::Resumed;
        event.animationId = animationId;
        event.timestamp = juce::Time::getHighResolutionTicks();
        notifyAnimationEvent(event);
    }
}

void ProfessionalAnimationEngine::stopAnimation(const juce::String& animationId)
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end())
    {
        it->second.state = AnimationState::Stopped;

        AnimationEvent event;
        event.type = AnimationEvent::EventType::Stopped;
        event.animationId = animationId;
        event.timestamp = juce::Time::getHighResolutionTicks();
        notifyAnimationEvent(event);
    }
}

void ProfessionalAnimationEngine::removeAnimation(const juce::String& animationId)
{
    pImpl->animations.erase(animationId);
    pImpl->physicsStates.erase(animationId);
}

//==============================================================================
ProfessionalAnimationEngine::AnimationState ProfessionalAnimationEngine::getAnimationState(const juce::String& animationId) const
{
    auto it = pImpl->animations.find(animationId);
    return it != pImpl->animations.end() ? it->second.state : AnimationState::Idle;
}

float ProfessionalAnimationEngine::getAnimationProgress(const juce::String& animationId) const
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end())
    {
        return juce::jlimit(0.0f, 1.0f, it->second.currentTime / it->second.config.duration);
    }
    return 0.0f;
}

float ProfessionalAnimationEngine::getAnimationValue(const juce::String& animationId) const
{
    auto it = pImpl->animations.find(animationId);
    return it != pImpl->animations.end() ? it->second.currentValue : 0.0f;
}

bool ProfessionalAnimationEngine::isAnimationActive(const juce::String& animationId) const
{
    auto it = pImpl->animations.find(animationId);
    return it != pImpl->animations.end() && it->second.state == AnimationState::Playing;
}

bool ProfessionalAnimationEngine::isAnimationCompleted(const juce::String& animationId) const
{
    auto it = pImpl->animations.find(animationId);
    return it != pImpl->animations.end() && it->second.state == AnimationState::Completed;
}

//==============================================================================
void ProfessionalAnimationEngine::setAnimationSpeed(const juce::String& animationId, float speed)
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end())
    {
        it->second.config.speed = juce::jmax(0.01f, speed);
    }
}

void ProfessionalAnimationEngine::setAnimationDuration(const juce::String& animationId, float duration)
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end())
    {
        it->second.config.duration = juce::jmax(0.01f, duration);
    }
}

void ProfessionalAnimationEngine::setAnimationEasing(const juce::String& animationId, EasingType easing)
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end())
    {
        it->second.config.easing = easing;
    }
}

//==============================================================================
void ProfessionalAnimationEngine::setAudioReactive(const juce::String& animationId, bool enabled)
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end())
    {
        it->second.config.audioReactive = enabled;
    }
}

void ProfessionalAnimationEngine::setAudioSensitivity(const juce::String& animationId, float sensitivity)
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end())
    {
        it->second.config.audioSensitivity = juce::jlimit(0.0f, 10.0f, sensitivity);
    }
}

void ProfessionalAnimationEngine::updateAudioAnalysis(const AudioAnalysis& analysis)
{
    pImpl->audioAnalysis = analysis;
    pImpl->audioLevel = analysis.level;
}

float ProfessionalAnimationEngine::getAudioLevel() const
{
    return pImpl->audioLevel;
}

//==============================================================================
void ProfessionalAnimationEngine::setPhysicsEnabled(const juce::String& animationId, bool enabled)
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end())
    {
        it->second.config.physicsEnabled = enabled;

        if (enabled)
        {
            auto& physics = pImpl->physicsStates[animationId];
            physics.position = it->second.currentValue;
            physics.targetPosition = it->second.target.endValue;
            physics.springStiffness = it->second.config.springStiffness;
            physics.springDamping = it->second.config.springDamping;
            physics.mass = it->second.config.mass;
        }
    }
}

void ProfessionalAnimationEngine::setSpringStiffness(const juce::String& animationId, float stiffness)
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end())
    {
        it->second.config.springStiffness = stiffness;
        auto physicsIt = pImpl->physicsStates.find(animationId);
        if (physicsIt != pImpl->physicsStates.end())
        {
            physicsIt->second.springStiffness = stiffness;
        }
    }
}

void ProfessionalAnimationEngine::setSpringDamping(const juce::String& animationId, float damping)
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end())
    {
        it->second.config.springDamping = damping;
        auto physicsIt = pImpl->physicsStates.find(animationId);
        if (physicsIt != pImpl->physicsStates.end())
        {
            physicsIt->second.springDamping = damping;
        }
    }
}

void ProfessionalAnimationEngine::setMass(const juce::String& animationId, float mass)
{
    auto it = pImpl->animations.find(animationId);
    if (it != pImpl->animations.end())
    {
        it->second.config.mass = mass;
        auto physicsIt = pImpl->physicsStates.find(animationId);
        if (physicsIt != pImpl->physicsStates.end())
        {
            physicsIt->second.mass = mass;
        }
    }
}

//==============================================================================
float ProfessionalAnimationEngine::applyEasing(float t, EasingType easing) const
{
    return calculateEasingValue(t, easing);
}

float ProfessionalAnimationEngine::calculateEasingValue(float t, EasingType easing) const
{
    t = juce::jlimit(0.0f, 1.0f, t);

    switch (easing)
    {
        case EasingType::Linear:
            return t;

        case EasingType::EaseIn:
            return t * t;

        case EasingType::EaseOut:
            return 1.0f - (1.0f - t) * (1.0f - t);

        case EasingType::EaseInOut:
            return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;

        case EasingType::EaseOutIn:
            return t < 0.5f ? (1.0f - std::pow(-2.0f * t + 1.0f, 2.0f)) / 2.0f :
                              (std::pow(2.0f * t - 1.0f, 2.0f) + 1.0f) / 2.0f;

        case EasingType::Bounce:
        {
            constexpr float n1 = 7.5625f;
            constexpr float d1 = 2.75f;

            if (t < 1.0f / d1)
                return n1 * t * t;
            else if (t < 2.0f / d1)
            {
                float tt = t - 1.5f / d1;
                return n1 * tt * tt + 0.75f;
            }
            else if (t < 2.5f / d1)
            {
                float tt = t - 2.25f / d1;
                return n1 * tt * tt + 0.9375f;
            }
            else
            {
                float tt = t - 2.625f / d1;
                return n1 * tt * tt + 0.984375f;
            }
        }

        case EasingType::Elastic:
        {
            constexpr float c4 = (2.0f * juce::MathConstants<float>::pi) / 3.0f;
            return t == 0.0f ? 0.0f : t == 1.0f ? 1.0f
                : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
        }

        case EasingType::Back:
        {
            constexpr float c1 = 1.70158f;
            constexpr float c3 = c1 + 1.0f;
            return c3 * t * t * t - c1 * t * t;
        }

        case EasingType::Sine:
            return 1.0f - std::cos((t * juce::MathConstants<float>::pi) / 2.0f);

        case EasingType::Quad:
            return t * t;

        case EasingType::Cubic:
            return t * t * t;

        case EasingType::Quart:
            return t * t * t * t;

        case EasingType::Quint:
            return t * t * t * t * t;

        case EasingType::Expo:
            return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));

        case EasingType::Circ:
            return 1.0f - std::sqrt(1.0f - t * t);

        default:
            return t;
    }
}

//==============================================================================
ProfessionalAnimationEngine::PerformanceMetrics ProfessionalAnimationEngine::getPerformanceMetrics() const
{
    return pImpl->performanceMetrics;
}

void ProfessionalAnimationEngine::setTargetFrameRate(float frameRate)
{
    pImpl->targetFrameRate = juce::jmax(30.0f, frameRate);
}

void ProfessionalAnimationEngine::setPerformanceMode(bool enabled)
{
    pImpl->performanceMode = enabled;
}

void ProfessionalAnimationEngine::optimizePerformance()
{
    // Reduce animation quality if performance is poor
    if (pImpl->performanceMetrics.frameRate < pImpl->targetFrameRate * 0.8f)
    {
        pImpl->performanceMode = true;
    }
}

void ProfessionalAnimationEngine::clearCompletedAnimations()
{
    auto it = pImpl->animations.begin();
    while (it != pImpl->animations.end())
    {
        if (it->second.state == AnimationState::Completed ||
            it->second.state == AnimationState::Error)
        {
            it = pImpl->animations.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

//==============================================================================
void ProfessionalAnimationEngine::addAnimationListener(AnimationCallback callback)
{
    pImpl->animationCallbacks.push_back(callback);
}

void ProfessionalAnimationEngine::removeAnimationListener(AnimationCallback callback)
{
    // std::function doesn't support operator== for general callables.
    // Prefer matching function pointers when possible; otherwise, match by target_type.
    if (auto fp = callback.template target<void(*)(const AnimationEvent&)>())
    {
        auto it = std::remove_if(pImpl->animationCallbacks.begin(), pImpl->animationCallbacks.end(), [&](const AnimationCallback& cb){
            if (auto other = cb.template target<void(*)(const AnimationEvent&)>())
                return *other == *fp;
            return false;
        });
        pImpl->animationCallbacks.erase(it, pImpl->animationCallbacks.end());
    }
    else
    {
    const std::type_info& type = callback.target_type();
        auto it = std::remove_if(pImpl->animationCallbacks.begin(), pImpl->animationCallbacks.end(), [&](const AnimationCallback& cb){
            return cb.target_type() == type;
        });
        pImpl->animationCallbacks.erase(it, pImpl->animationCallbacks.end());
    }
}

//==============================================================================
juce::String ProfessionalAnimationEngine::generateAnimationId() const
{
    return "anim_" + juce::String(++pImpl->animationCounter);
}

void ProfessionalAnimationEngine::resetAllAnimations()
{
    for (auto& [id, animation] : pImpl->animations)
    {
        animation.state = AnimationState::Idle;
        animation.currentTime = 0.0f;
        animation.currentValue = animation.target.startValue;
    }
}

void ProfessionalAnimationEngine::pauseAllAnimations()
{
    for (auto& [id, animation] : pImpl->animations)
    {
        if (animation.state == AnimationState::Playing)
        {
            animation.state = AnimationState::Paused;
        }
    }
}

void ProfessionalAnimationEngine::resumeAllAnimations()
{
    for (auto& [id, animation] : pImpl->animations)
    {
        if (animation.state == AnimationState::Paused)
        {
            animation.state = AnimationState::Playing;
        }
    }
}

void ProfessionalAnimationEngine::stopAllAnimations()
{
    for (auto& [id, animation] : pImpl->animations)
    {
        animation.state = AnimationState::Stopped;
    }
}

//==============================================================================
void ProfessionalAnimationEngine::updateAnimation(AnimationInstance& animation, float deltaTime)
{
    if (animation.state != AnimationState::Playing)
        return;

    // Handle delay
    if (animation.currentTime < animation.config.delay)
    {
        animation.currentTime += deltaTime * animation.config.speed;
        return;
    }

    // Update time
    float effectiveTime = animation.currentTime - animation.config.delay;
    animation.currentTime += deltaTime * animation.config.speed;

    // Calculate progress
    float progress = 0.0f;

    if (animation.config.useKeyframes && !animation.config.keyframeTimes.empty())
    {
        progress = interpolateKeyframes(effectiveTime, animation.config.keyframeTimes, animation.config.keyframeValues);
    }
    else if (animation.config.physicsEnabled)
    {
        // Physics-based animation
        auto& physics = pImpl->physicsStates[animation.id];
        physics.targetPosition = animation.target.endValue;

        float force = (physics.targetPosition - physics.position) * physics.springStiffness;
        float dampingForce = physics.velocity * physics.springDamping;
        float acceleration = (force - dampingForce) / physics.mass;

        physics.velocity += acceleration * deltaTime;
        physics.position += physics.velocity * deltaTime;

        progress = (physics.position - animation.target.startValue) /
                   (animation.target.endValue - animation.target.startValue);
    }
    else
    {
        // Normal easing-based animation
        float normalizedTime = effectiveTime / animation.config.duration;
        normalizedTime = juce::jlimit(0.0f, 1.0f, normalizedTime);

        if (animation.config.audioReactive)
        {
            // Apply audio modulation
            float audioMod = pImpl->audioLevel * animation.config.audioSensitivity;
            normalizedTime = juce::jlimit(0.0f, 1.0f, normalizedTime + audioMod * 0.1f);
        }

        progress = calculateEasingValue(normalizedTime, animation.config.easing);
    }

    // Apply auto-reverse when a cycle finishes
    if (animation.config.autoReverse && (effectiveTime >= animation.config.duration))
    {
        animation.isReversed = !animation.isReversed;
        animation.currentTime = animation.config.delay;
        progress = 0.0f;
    }

    if (animation.isReversed)
    {
        progress = 1.0f - progress;
    }

    // Calculate value
    animation.currentValue = animation.target.startValue +
                            (animation.target.endValue - animation.target.startValue) * progress;

    // Apply value
    applyAnimationValue(animation, animation.currentValue);

    // Check completion
    if (effectiveTime >= animation.config.duration)
    {
        if (animation.config.infinite)
        {
            animation.currentTime = animation.config.delay;
        }
        else if (animation.config.repeatCount > 0 && animation.currentRepeat < animation.config.repeatCount)
        {
            animation.currentRepeat++;
            animation.currentTime = animation.config.delay;
        }
        else
        {
            animation.state = AnimationState::Completed;
            animation.currentValue = animation.target.endValue;
            applyAnimationValue(animation, animation.currentValue);

            AnimationEvent event;
            event.type = AnimationEvent::EventType::Completed;
            event.animationId = animation.id;
            event.value = animation.currentValue;
            event.timestamp = juce::Time::getHighResolutionTicks();
            notifyAnimationEvent(event);

            if (animation.onComplete)
                animation.onComplete(animation);
        }
    }
    else
    {
        AnimationEvent event;
        event.type = AnimationEvent::EventType::Updated;
        event.animationId = animation.id;
        event.value = animation.currentValue;
        event.time = effectiveTime;
        event.timestamp = juce::Time::getHighResolutionTicks();
        notifyAnimationEvent(event);

        if (animation.onUpdate)
            animation.onUpdate(animation);
    }
}

void ProfessionalAnimationEngine::applyAnimationValue(const AnimationInstance& animation, float value)
{
    switch (animation.config.type)
    {
        case AnimationType::Position:
            if (animation.target.component)
            {
                auto pos = animation.target.startPosition +
                          (animation.target.endPosition - animation.target.startPosition) * value;
                animation.target.component->setTopLeftPosition(pos.toInt());
            }
            break;

        case AnimationType::Scale:
            if (animation.target.component)
            {
                auto scale = animation.target.startScale +
                            (animation.target.endScale - animation.target.startScale) * value;
                animation.target.component->setTransform(
                    juce::AffineTransform::scale(scale, scale));
            }
            break;

        case AnimationType::Opacity:
            if (animation.target.component)
            {
                float opacity = animation.target.startOpacity +
                               (animation.target.endOpacity - animation.target.startOpacity) * value;
                animation.target.component->setAlpha(opacity);
            }
            break;

        case AnimationType::Custom:
            if (animation.target.customCallback)
            {
                animation.target.customCallback(value);
            }
            break;

        default:
            break;
    }
}

void ProfessionalAnimationEngine::notifyAnimationEvent(const AnimationEvent& event)
{
    for (auto& callback : pImpl->animationCallbacks)
    {
        callback(event);
    }
}

float ProfessionalAnimationEngine::interpolateKeyframes(float time, const std::vector<float>& times, const std::vector<float>& values) const
{
    if (times.empty() || values.empty() || times.size() != values.size())
        return 0.0f;

    // Find surrounding keyframes
    for (size_t i = 0; i < times.size() - 1; ++i)
    {
        if (time >= times[i] && time <= times[i + 1])
        {
            // Linear interpolation
            float t = (time - times[i]) / (times[i + 1] - times[i]);
            return values[i] + (values[i + 1] - values[i]) * t;
        }
    }

    // Clamp to first/last keyframe
    if (time <= times[0])
        return values[0];
    else
        return values.back();
}

} // namespace daw::ui::animation

