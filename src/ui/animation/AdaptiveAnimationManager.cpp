#include "AdaptiveAnimationManager.h"
#include "../../core/ServiceLocator.h"
#include <algorithm>
#include <cmath>

namespace daw::ui {

AdaptiveAnimationManager::AdaptiveAnimationManager()
{
    fpsHistory.reserve(60); // Store 1 second of FPS history at 60fps
}

AdaptiveAnimationManager::~AdaptiveAnimationManager()
{
    shutdown();
}

void AdaptiveAnimationManager::attachToComponent(juce::Component& component)
{
    if (!openGLContext)
        openGLContext = std::make_unique<juce::OpenGLContext>();

    if (attachedComponent == &component)
        return;

    detachFromComponent(component);

    attachedComponent = &component;
    openGLContext->setRenderer(this);
    openGLContext->attachTo(component);
    openGLContext->setContinuousRepainting(false);
}

void AdaptiveAnimationManager::detachFromComponent(juce::Component& component)
{
    if (attachedComponent != &component)
        return;

    if (openGLContext)
        openGLContext->detach();

    attachedComponent = nullptr;
}

//==============================================================================
// Service Lifecycle
//==============================================================================

bool AdaptiveAnimationManager::initialize()
{
    if (initialized.load())
        return true;

    juce::Logger::writeToLog("Initializing Adaptive Animation Manager...");

    // Initialize animation pool
    initializeAnimationPool();

    // Check for GPU acceleration support
    auto& locator = core::ServiceLocator::getInstance();
    const bool gpuEnabled = locator.getFeatureFlag("gpu");

    if (gpuEnabled)
    {
        openGLContext = std::make_unique<juce::OpenGLContext>();
        openGLContext->setRenderer(this);
        openGLContext->setContinuousRepainting(false); // Only repaint when needed

        // Try to attach to a component (this would typically be done by the main UI)
        // For now, we'll set up for potential attachment
        performanceMetrics.gpuAccelerated.store(false); // Will be set to true in newOpenGLContextCreated
    }

    // Start the timer for animation updates
    const int timerIntervalMs = static_cast<int>(1000.0f / targetFPS.load());
    startTimer(timerIntervalMs);

    lastFrameTime = std::chrono::steady_clock::now();
    initialized.store(true);

    juce::Logger::writeToLog("Adaptive Animation Manager initialized");
    return true;
}

void AdaptiveAnimationManager::shutdown()
{
    if (!initialized.load())
        return;

    juce::Logger::writeToLog("Shutting down Adaptive Animation Manager...");

    // Stop the timer
    stopTimer();

    // Cancel all animations
    cancelAllAnimations();

    // Release OpenGL resources
    if (openGLContext)
    {
        openGLContext->detach();
        openGLContext.reset();
    }

    // Clear animation pool
    {
        std::lock_guard<std::mutex> lock(animationMutex);
        animationPool.clear();
        activeAnimations.clear();
        availableAnimations.clear();
    }

    initialized.store(false);
    juce::Logger::writeToLog("Adaptive Animation Manager shutdown complete");
}

//==============================================================================
// Animation Creation and Control
//==============================================================================

uint32_t AdaptiveAnimationManager::createAnimation(
    float startValue,
    float endValue,
    float durationMs,
    AnimationType type)
{
    if (!initialized.load())
        return 0;

    auto* animation = allocateAnimation();
    if (!animation)
        return 0;

    const uint32_t animationId = nextAnimationId.fetch_add(1);

    animation->id.store(animationId);
    animation->startValue.store(startValue);
    animation->endValue.store(endValue);
    animation->currentValue.store(startValue);
    animation->duration.store(durationMs);
    animation->elapsed.store(0.0f);
    animation->type.store(type);
    animation->state.store(AnimationState::Idle);
    animation->delay.store(0.0f);
    animation->repeatCount.store(1);
    animation->autoReverse.store(false);
    animation->speedMultiplier.store(1.0f);

    // Add to active animations
    {
        std::lock_guard<std::mutex> lock(animationMutex);
        activeAnimations[animationId] = animation;
    }

    return animationId;
}

uint32_t AdaptiveAnimationManager::createCubicBezierAnimation(
    float startValue,
    float endValue,
    float durationMs,
    float p1x, float p1y, float p2x, float p2y)
{
    uint32_t animationId = createAnimation(startValue, endValue, durationMs, AnimationType::CubicBezier);

    if (animationId != 0)
    {
        std::lock_guard<std::mutex> lock(animationMutex);
        auto it = activeAnimations.find(animationId);
        if (it != activeAnimations.end())
        {
            auto* animation = it->second;
            animation->bezierP1x = clamp(p1x, 0.0f, 1.0f);
            animation->bezierP1y = p1y;
            animation->bezierP2x = clamp(p2x, 0.0f, 1.0f);
            animation->bezierP2y = p2y;
        }
    }

    return animationId;
}

bool AdaptiveAnimationManager::startAnimation(uint32_t animationId)
{
    std::lock_guard<std::mutex> lock(animationMutex);
    auto it = activeAnimations.find(animationId);
    if (it == activeAnimations.end())
        return false;

    auto* animation = it->second;
    animation->state.store(AnimationState::Running);
    animation->startTime = std::chrono::steady_clock::now();
    animation->elapsed.store(0.0f);

    performanceMetrics.activeAnimations.fetch_add(1);
    return true;
}

bool AdaptiveAnimationManager::pauseAnimation(uint32_t animationId)
{
    std::lock_guard<std::mutex> lock(animationMutex);
    auto it = activeAnimations.find(animationId);
    if (it == activeAnimations.end())
        return false;

    auto* animation = it->second;
    if (animation->state.load() == AnimationState::Running)
    {
        animation->state.store(AnimationState::Paused);
        performanceMetrics.activeAnimations.fetch_sub(1);
        return true;
    }

    return false;
}

bool AdaptiveAnimationManager::stopAnimation(uint32_t animationId)
{
    std::lock_guard<std::mutex> lock(animationMutex);
    auto it = activeAnimations.find(animationId);
    if (it == activeAnimations.end())
        return false;

    auto* animation = it->second;
    const auto previousState = animation->state.load();

    animation->state.store(AnimationState::Idle);
    animation->elapsed.store(0.0f);
    animation->currentValue.store(animation->startValue.load());

    if (previousState == AnimationState::Running)
    {
        performanceMetrics.activeAnimations.fetch_sub(1);
    }

    return true;
}

bool AdaptiveAnimationManager::cancelAnimation(uint32_t animationId)
{
    std::lock_guard<std::mutex> lock(animationMutex);
    auto it = activeAnimations.find(animationId);
    if (it == activeAnimations.end())
        return false;

    auto* animation = it->second;
    const auto previousState = animation->state.load();

    animation->state.store(AnimationState::Cancelled);

    if (previousState == AnimationState::Running)
    {
        performanceMetrics.activeAnimations.fetch_sub(1);
    }

    // Mark for cleanup (will be deallocated on next update)
    return true;
}

void AdaptiveAnimationManager::cancelAllAnimations()
{
    std::lock_guard<std::mutex> lock(animationMutex);

    for (auto& pair : activeAnimations)
    {
        auto* animation = pair.second;
        if (animation->state.load() == AnimationState::Running)
        {
            performanceMetrics.activeAnimations.fetch_sub(1);
        }
        animation->state.store(AnimationState::Cancelled);
    }
}

//==============================================================================
// Animation Configuration
//==============================================================================

bool AdaptiveAnimationManager::setAnimationCallback(uint32_t animationId, std::function<void(float)> callback)
{
    std::lock_guard<std::mutex> lock(animationMutex);
    auto it = activeAnimations.find(animationId);
    if (it == activeAnimations.end())
        return false;

    it->second->valueCallback = std::move(callback);
    return true;
}

bool AdaptiveAnimationManager::setCompletionCallback(uint32_t animationId, std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(animationMutex);
    auto it = activeAnimations.find(animationId);
    if (it == activeAnimations.end())
        return false;

    it->second->completionCallback = std::move(callback);
    return true;
}

bool AdaptiveAnimationManager::setAnimationDelay(uint32_t animationId, float delayMs)
{
    std::lock_guard<std::mutex> lock(animationMutex);
    auto it = activeAnimations.find(animationId);
    if (it == activeAnimations.end())
        return false;

    it->second->delay.store(std::max(0.0f, delayMs));
    return true;
}

bool AdaptiveAnimationManager::setAnimationRepeat(uint32_t animationId, int count, bool autoReverse)
{
    std::lock_guard<std::mutex> lock(animationMutex);
    auto it = activeAnimations.find(animationId);
    if (it == activeAnimations.end())
        return false;

    auto* animation = it->second;
    animation->repeatCount.store(std::max(1, count));
    animation->autoReverse.store(autoReverse);
    return true;
}

bool AdaptiveAnimationManager::setAnimationSpeed(uint32_t animationId, float speedMultiplier)
{
    std::lock_guard<std::mutex> lock(animationMutex);
    auto it = activeAnimations.find(animationId);
    if (it == activeAnimations.end())
        return false;

    it->second->speedMultiplier.store(std::max(0.1f, speedMultiplier));
    return true;
}

//==============================================================================
// Animation Queries
//==============================================================================

bool AdaptiveAnimationManager::isAnimationRunning(uint32_t animationId) const
{
    std::lock_guard<std::mutex> lock(animationMutex);
    auto it = activeAnimations.find(animationId);
    return (it != activeAnimations.end()) &&
           (it->second->state.load() == AnimationState::Running);
}

float AdaptiveAnimationManager::getAnimationProgress(uint32_t animationId) const
{
    std::lock_guard<std::mutex> lock(animationMutex);
    auto it = activeAnimations.find(animationId);
    if (it == activeAnimations.end())
        return 0.0f;

    const auto* animation = it->second;
    const float duration = animation->duration.load();
    const float elapsed = animation->elapsed.load();

    return (duration > 0.0f) ? clamp(elapsed / duration, 0.0f, 1.0f) : 1.0f;
}

float AdaptiveAnimationManager::getAnimationValue(uint32_t animationId) const
{
    std::lock_guard<std::mutex> lock(animationMutex);
    auto it = activeAnimations.find(animationId);
    return (it != activeAnimations.end()) ? it->second->currentValue.load() : 0.0f;
}

AdaptiveAnimationManager::AnimationState AdaptiveAnimationManager::getAnimationState(uint32_t animationId) const
{
    std::lock_guard<std::mutex> lock(animationMutex);
    auto it = activeAnimations.find(animationId);
    return (it != activeAnimations.end()) ? it->second->state.load() : AnimationState::Idle;
}

//==============================================================================
// Timer Callback
//==============================================================================

void AdaptiveAnimationManager::timerCallback()
{
    updateAnimations();
    updatePerformanceMetrics();

    if (adaptiveQuality.load())
    {
        adjustQualityLevel();
    }

    cleanupCompletedAnimations();
}

//==============================================================================
// OpenGL Integration
//==============================================================================

void AdaptiveAnimationManager::newOpenGLContextCreated()
{
    performanceMetrics.gpuAccelerated.store(true);
    setupGLResources();
    juce::Logger::writeToLog("Animation Manager: OpenGL context created, GPU acceleration enabled");
}

void AdaptiveAnimationManager::openGLContextClosing()
{
    releaseGLResources();
    performanceMetrics.gpuAccelerated.store(false);
    juce::Logger::writeToLog("Animation Manager: OpenGL context closing, falling back to CPU");
}

void AdaptiveAnimationManager::renderOpenGL()
{
    // GPU-accelerated animation rendering would go here
    // For now, we rely on CPU-based animation updates
}

//==============================================================================
// Internal Methods
//==============================================================================

void AdaptiveAnimationManager::updateAnimations()
{
    const auto currentTime = std::chrono::steady_clock::now();
    const float deltaTimeMs = std::chrono::duration_cast<std::chrono::microseconds>(
        currentTime - lastFrameTime).count() / 1000.0f;
    lastFrameTime = currentTime;

    std::lock_guard<std::mutex> lock(animationMutex);

    for (auto& pair : activeAnimations)
    {
        auto* animation = pair.second;
        if (animation->state.load() == AnimationState::Running)
        {
            processAnimation(animation, deltaTimeMs);
        }
    }
}

void AdaptiveAnimationManager::processAnimation(AnimationData* animation, float deltaTimeMs)
{
    const float speedMultiplier = animation->speedMultiplier.load();
    const float adjustedDelta = deltaTimeMs * speedMultiplier;

    // Handle delay
    const float delay = animation->delay.load();
    if (delay > 0.0f)
    {
        animation->delay.store(std::max(0.0f, delay - adjustedDelta));
        return;
    }

    // Update elapsed time
    const float elapsed = animation->elapsed.load() + adjustedDelta;
    animation->elapsed.store(elapsed);

    const float duration = animation->duration.load();
    const float progress = (duration > 0.0f) ? std::min(elapsed / duration, 1.0f) : 1.0f;

    // Calculate eased progress
    const float easedProgress = calculateEasing(progress, animation->type.load());

    // Interpolate value
    const float startValue = animation->startValue.load();
    const float endValue = animation->endValue.load();
    const float currentValue = lerp(startValue, endValue, easedProgress);
    animation->currentValue.store(currentValue);

    // Call value callback on message thread
    if (animation->valueCallback)
    {
        juce::MessageManager::callAsync([callback = animation->valueCallback, currentValue]()
        {
            callback(currentValue);
        });
    }

    // Check if animation is complete
    if (progress >= 1.0f)
    {
        animation->state.store(AnimationState::Completed);
        performanceMetrics.activeAnimations.fetch_sub(1);
        performanceMetrics.completedAnimations.fetch_add(1);

        // Call completion callback on message thread
        if (animation->completionCallback)
        {
            juce::MessageManager::callAsync([callback = animation->completionCallback]()
            {
                callback();
            });
        }
    }
}

float AdaptiveAnimationManager::calculateEasing(float progress, AnimationType type) const
{
    switch (type)
    {
        case AnimationType::Linear:
            return progress;

        case AnimationType::EaseIn:
            return progress * progress;

        case AnimationType::EaseOut:
            return 1.0f - (1.0f - progress) * (1.0f - progress);

        case AnimationType::EaseInOut:
            return (progress < 0.5f) ?
                2.0f * progress * progress :
                1.0f - 2.0f * (1.0f - progress) * (1.0f - progress);

        case AnimationType::Bounce:
        {
            const float n1 = 7.5625f;
            const float d1 = 2.75f;

            if (progress < 1.0f / d1)
                return n1 * progress * progress;
            if (progress < 2.0f / d1)
            {
                const float adjusted = progress - (1.5f / d1);
                return n1 * adjusted * adjusted + 0.75f;
            }
            if (progress < 2.5f / d1)
            {
                const float adjusted = progress - (2.25f / d1);
                return n1 * adjusted * adjusted + 0.9375f;
            }

            const float adjusted = progress - (2.625f / d1);
            return n1 * adjusted * adjusted + 0.984375f;
        }

        case AnimationType::Elastic:
        {
            const float c4 = (2.0f * juce::MathConstants<float>::pi) / 3.0f;

            if (progress == 0.0f) return 0.0f;
            if (progress == 1.0f) return 1.0f;

            return -std::pow(2.0f, 10.0f * progress - 10.0f) * std::sin((progress * 10.0f - 10.75f) * c4);
        }

        case AnimationType::Spring:
        {
            const float tension = 0.8f;
            const float friction = 0.3f;
            return 1.0f - std::exp(-tension * progress) * std::cos(friction * progress);
        }

        case AnimationType::CubicBezier:
            // This would need access to the specific animation's bezier parameters
            // For now, default to ease-in-out
            return calculateEasing(progress, AnimationType::EaseInOut);

        default:
            return progress;
    }
}

float AdaptiveAnimationManager::calculateCubicBezier(float t, float p1x, float p1y, float p2x, float p2y) const
{
    const float u = 1.0f - t;
    const float tt = t * t;
    const float uu = u * u;
    const float ttt = tt * t;

    const float x = (3.0f * uu * t * p1x) + (3.0f * u * tt * p2x) + ttt;
    juce::ignoreUnused(x); // X would map to time if we solved for parametric curves.

    return (3.0f * uu * t * p1y) + (3.0f * u * tt * p2y) + ttt;
}

AdaptiveAnimationManager::AnimationData* AdaptiveAnimationManager::allocateAnimation()
{
    std::lock_guard<std::mutex> lock(animationMutex);

    if (availableAnimations.empty())
    {
        if (animationPool.size() >= ANIMATION_POOL_SIZE)
            return nullptr; // Pool exhausted

        // Create new animation
        auto animation = std::make_unique<AnimationData>();
        auto* ptr = animation.get();
        animationPool.push_back(std::move(animation));
        return ptr;
    }

    auto* animation = availableAnimations.back();
    availableAnimations.pop_back();
    animation->inUse.store(true);
    return animation;
}

void AdaptiveAnimationManager::deallocateAnimation(AdaptiveAnimationManager::AnimationData* animation)
{
    if (!animation)
        return;

    // Reset animation data
    animation->inUse.store(false);
    animation->state.store(AnimationState::Idle);
    animation->valueCallback = nullptr;
    animation->completionCallback = nullptr;

    std::lock_guard<std::mutex> lock(animationMutex);
    availableAnimations.push_back(animation);
}

void AdaptiveAnimationManager::initializeAnimationPool()
{
    std::lock_guard<std::mutex> lock(animationMutex);

    animationPool.reserve(ANIMATION_POOL_SIZE);
    availableAnimations.reserve(ANIMATION_POOL_SIZE);

    // Pre-allocate some animations for immediate use
    const int preAllocCount = std::min(50, ANIMATION_POOL_SIZE);
    for (int i = 0; i < preAllocCount; ++i)
    {
        auto animation = std::make_unique<AnimationData>();
        availableAnimations.push_back(animation.get());
        animationPool.push_back(std::move(animation));
    }
}

void AdaptiveAnimationManager::cleanupCompletedAnimations()
{
    std::lock_guard<std::mutex> lock(animationMutex);

    auto it = activeAnimations.begin();
    while (it != activeAnimations.end())
    {
        auto* animation = it->second;
        const auto state = animation->state.load();

        if (state == AnimationState::Completed || state == AnimationState::Cancelled)
        {
            deallocateAnimation(animation);
            it = activeAnimations.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void AdaptiveAnimationManager::updatePerformanceMetrics()
{
    const auto currentTime = std::chrono::steady_clock::now();
    const float deltaMs = std::chrono::duration_cast<std::chrono::microseconds>(
        currentTime - lastFrameTime).count() / 1000.0f;

    if (deltaMs > 0.0f)
    {
        const float currentFPS = 1000.0f / deltaMs;
        performanceMetrics.currentFPS.store(currentFPS);
        performanceMetrics.frameTimeMs.store(deltaMs);

        // Update FPS history for averaging
        fpsHistory.push_back(currentFPS);
        if (fpsHistory.size() > 60)
        {
            fpsHistory.erase(fpsHistory.begin());
        }

        // Calculate average FPS
        if (!fpsHistory.empty())
        {
            const float avgFPS = std::accumulate(fpsHistory.begin(), fpsHistory.end(), 0.0f) / fpsHistory.size();
            performanceMetrics.averageFPS.store(avgFPS);
        }

        // Count dropped frames
        if (currentFPS < targetFPS.load() * 0.9f) // 10% tolerance
        {
            performanceMetrics.droppedFrames.fetch_add(1);
        }
    }

    // Update memory usage estimate
    const size_t memoryUsage = animationPool.size() * sizeof(AnimationData);
    performanceMetrics.memoryUsageBytes.store(memoryUsage);
}

void AdaptiveAnimationManager::adjustQualityLevel()
{
    const float avgFPS = performanceMetrics.averageFPS.load();
    const float targetFPS = this->targetFPS.load();

    QualityLevel newLevel = currentQualityLevel.load();

    if (avgFPS < targetFPS * 0.5f) // Less than 50% of target
    {
        newLevel = QualityLevel::UltraLow;
    }
    else if (avgFPS < targetFPS * 0.7f) // Less than 70% of target
    {
        newLevel = QualityLevel::Low;
    }
    else if (avgFPS < targetFPS * 0.85f) // Less than 85% of target
    {
        newLevel = QualityLevel::Medium;
    }
    else if (avgFPS < targetFPS * 0.95f) // Less than 95% of target
    {
        newLevel = QualityLevel::High;
    }
    else
    {
        newLevel = QualityLevel::UltraHigh;
    }

    if (newLevel != currentQualityLevel.load())
    {
        currentQualityLevel.store(newLevel);
        optimizeForPerformance();
    }
}

void AdaptiveAnimationManager::optimizeForPerformance()
{
    const auto qualityLevel = currentQualityLevel.load();

    // Adjust timer frequency based on quality level
    int newInterval = 16;
    switch (qualityLevel)
    {
        case QualityLevel::UltraHigh:
            newInterval = 16; // ~60fps
            break;
        case QualityLevel::High:
            newInterval = 20; // ~50fps
            break;
        case QualityLevel::Medium:
            newInterval = 25; // ~40fps
            break;
        case QualityLevel::Low:
            newInterval = 33; // ~30fps
            break;
        case QualityLevel::UltraLow:
            newInterval = 50; // ~20fps
            break;
    }

    if (getTimerInterval() != newInterval)
    {
        startTimer(newInterval);
        juce::Logger::writeToLog("Animation quality adjusted to level " + juce::String((int)qualityLevel));
    }
}

void AdaptiveAnimationManager::setupGLResources()
{
    // Setup OpenGL resources for GPU-accelerated animations
    // This would include creating shaders, vertex buffers, etc.
    compileShaders();
}

void AdaptiveAnimationManager::releaseGLResources()
{
    // Release OpenGL resources
    animationShader.reset();
}

void AdaptiveAnimationManager::compileShaders()
{
    // Simple shader for GPU-accelerated animation rendering
    const char* vertexShader = R"(
        attribute vec4 position;
        attribute vec2 texCoord;
        varying vec2 vTexCoord;
        uniform mat4 projectionMatrix;

        void main()
        {
            vTexCoord = texCoord;
            gl_Position = projectionMatrix * position;
        }
    )";

    const char* fragmentShader = R"(
        varying vec2 vTexCoord;
        uniform float animationProgress;
        uniform vec4 color;

        void main()
        {
            float alpha = color.a * animationProgress;
            gl_FragColor = vec4(color.rgb, alpha);
        }
    )";

    animationShader = std::make_unique<juce::OpenGLShaderProgram>(*openGLContext);

    if (!animationShader->addVertexShader(vertexShader) ||
        !animationShader->addFragmentShader(fragmentShader) ||
        !animationShader->link())
    {
        juce::Logger::writeToLog("Failed to compile animation shaders: " + animationShader->getLastError());
        animationShader.reset();
    }
}

float AdaptiveAnimationManager::clamp(float value, float min, float max) const
{
    return std::max(min, std::min(max, value));
}

float AdaptiveAnimationManager::lerp(float a, float b, float t) const
{
    return a + t * (b - a);
}

} // namespace daw::ui
