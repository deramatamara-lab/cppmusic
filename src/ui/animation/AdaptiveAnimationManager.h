#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include "../../core/ServiceLocator.h"
#include <atomic>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace daw::ui {

/**
 * Adaptive Animation Manager for premium DAW UI micro-interactions.
 *
 * Features:
 * - 60+ FPS performance with automatic quality scaling
 * - GPU acceleration via OpenGL where available
 * - Lock-free animation updates for real-time safety
 * - Memory pool for zero-allocation animation processing
 * - Performance monitoring and adaptive optimization
 * - Advanced easing curves with cubic Bezier support
 */
class AdaptiveAnimationManager : public juce::Timer,
                                public juce::OpenGLRenderer
{
public:
    // Performance targets
    static constexpr float TARGET_FPS = 60.0f;
    static constexpr float MIN_FPS = 30.0f;
    static constexpr int MAX_ANIMATIONS = 500;  // Reduced from oldbutgold for stability
    static constexpr int ANIMATION_POOL_SIZE = 1000;

    enum class AnimationType {
        Linear,
        EaseInOut,
        EaseIn,
        EaseOut,
        Bounce,
        Elastic,
        Spring,
        CubicBezier
    };

    enum class AnimationState {
        Idle,
        Running,
        Paused,
        Completed,
        Cancelled
    };

    enum class QualityLevel {
        UltraHigh = 0,  // 60+ FPS, full effects
        High = 1,       // 45-60 FPS, high quality
        Medium = 2,     // 30-45 FPS, medium quality
        Low = 3,        // 15-30 FPS, reduced effects
        UltraLow = 4    // <15 FPS, minimal effects
    };

    /**
     * Animation data structure optimized for cache performance
     */
    struct AnimationData
    {
        std::atomic<uint32_t> id{0};
        std::atomic<float> startValue{0.0f};
        std::atomic<float> endValue{1.0f};
        std::atomic<float> currentValue{0.0f};
        std::atomic<float> duration{300.0f}; // milliseconds
        std::atomic<float> elapsed{0.0f};
        std::atomic<AnimationType> type{AnimationType::EaseInOut};
        std::atomic<AnimationState> state{AnimationState::Idle};

        // Callbacks (set once, called from message thread)
        std::function<void(float)> valueCallback;
        std::function<void()> completionCallback;

        // Advanced properties
        std::atomic<float> delay{0.0f};
        std::atomic<int> repeatCount{1};
        std::atomic<bool> autoReverse{false};
        std::atomic<float> speedMultiplier{1.0f};

        // Cubic Bezier parameters
        float bezierP1x = 0.25f, bezierP1y = 0.1f;
        float bezierP2x = 0.25f, bezierP2y = 1.0f;

        // Timing
        std::chrono::steady_clock::time_point startTime;
        std::atomic<bool> inUse{false};
    };

    /**
     * Performance metrics for monitoring and optimization
     */
    struct PerformanceMetrics
    {
        std::atomic<float> currentFPS{60.0f};
        std::atomic<float> averageFPS{60.0f};
        std::atomic<int> activeAnimations{0};
        std::atomic<int> completedAnimations{0};
        std::atomic<float> frameTimeMs{16.67f};
        std::atomic<int> droppedFrames{0};
        std::atomic<bool> gpuAccelerated{false};
        std::atomic<size_t> memoryUsageBytes{0};
    };

    AdaptiveAnimationManager();
    ~AdaptiveAnimationManager() override;

    //==============================================================================
    // Service Lifecycle
    //==============================================================================

    bool initialize();
    void shutdown();
    [[nodiscard]] bool isInitialized() const { return initialized.load(); }

    //==============================================================================
    // Animation Creation and Control
    //==============================================================================

    /**
     * Create a new animation
     * @param startValue Starting value
     * @param endValue Target value
     * @param durationMs Animation duration in milliseconds
     * @param type Easing type
     * @return Animation ID, 0 if creation failed
     */
    [[nodiscard]] uint32_t createAnimation(
        float startValue,
        float endValue,
        float durationMs,
        AnimationType type = AnimationType::EaseInOut);

    /**
     * Create animation with custom cubic Bezier curve
     */
    [[nodiscard]] uint32_t createCubicBezierAnimation(
        float startValue,
        float endValue,
        float durationMs,
        float p1x, float p1y, float p2x, float p2y);

    /**
     * Start an animation
     */
    bool startAnimation(uint32_t animationId);

    /**
     * Pause an animation
     */
    bool pauseAnimation(uint32_t animationId);

    /**
     * Stop and reset an animation
     */
    bool stopAnimation(uint32_t animationId);

    /**
     * Cancel an animation (no completion callback)
     */
    bool cancelAnimation(uint32_t animationId);

    /**
     * Cancel all animations
     */
    void cancelAllAnimations();

    //==============================================================================
    // Animation Configuration
    //==============================================================================

    /**
     * Set value change callback
     */
    bool setAnimationCallback(uint32_t animationId, std::function<void(float)> callback);

    /**
     * Set completion callback
     */
    bool setCompletionCallback(uint32_t animationId, std::function<void()> callback);

    /**
     * Set animation delay
     */
    bool setAnimationDelay(uint32_t animationId, float delayMs);

    /**
     * Set repeat properties
     */
    bool setAnimationRepeat(uint32_t animationId, int count, bool autoReverse = false);

    /**
     * Set speed multiplier
     */
    bool setAnimationSpeed(uint32_t animationId, float speedMultiplier);

    //==============================================================================
    // Animation Queries
    //==============================================================================

    [[nodiscard]] bool isAnimationRunning(uint32_t animationId) const;
    [[nodiscard]] float getAnimationProgress(uint32_t animationId) const;
    [[nodiscard]] float getAnimationValue(uint32_t animationId) const;
    [[nodiscard]] AnimationState getAnimationState(uint32_t animationId) const;

    //==============================================================================
    // Performance Management
    //==============================================================================

    const PerformanceMetrics& getPerformanceMetrics() const { return performanceMetrics; }
    QualityLevel getCurrentQualityLevel() const { return currentQualityLevel.load(); }

    void setAdaptiveQuality(bool enabled) { adaptiveQuality.store(enabled); }
    [[nodiscard]] bool isAdaptiveQualityEnabled() const { return adaptiveQuality.load(); }

    void setTargetFPS(float fps) { targetFPS.store(fps); }
    [[nodiscard]] float getTargetFPS() const { return targetFPS.load(); }

    //==============================================================================
    // Component Attachment
    //==============================================================================

    /**
     * Attach the animation manager to a component for GPU rendering.
     */
    void attachToComponent(juce::Component& component);

    /**
     * Detach from a previously attached component.
     */
    void detachFromComponent(juce::Component& component);

    //==============================================================================
    // OpenGL Integration
    //==============================================================================

    void newOpenGLContextCreated() override;
    void openGLContextClosing() override;
    void renderOpenGL() override;

    //==============================================================================
    // Timer Callback
    //==============================================================================

    void timerCallback() override;

private:
    std::atomic<bool> initialized{false};
    std::atomic<bool> adaptiveQuality{true};
    std::atomic<float> targetFPS{60.0f};
    std::atomic<QualityLevel> currentQualityLevel{QualityLevel::UltraHigh};

    // Animation storage
    std::vector<std::unique_ptr<AnimationData>> animationPool;
    std::unordered_map<uint32_t, AnimationData*> activeAnimations;
    std::vector<AnimationData*> availableAnimations;
    mutable std::mutex animationMutex;

    std::atomic<uint32_t> nextAnimationId{1};

    // Performance tracking
    PerformanceMetrics performanceMetrics;
    std::chrono::steady_clock::time_point lastFrameTime;
    std::vector<float> fpsHistory;

    // OpenGL resources
    std::unique_ptr<juce::OpenGLShaderProgram> animationShader;
    std::unique_ptr<juce::OpenGLContext> openGLContext;
    juce::Component* attachedComponent { nullptr };

    //==============================================================================
    // Internal Methods
    //==============================================================================

    // Animation processing
    void updateAnimations();
    void processAnimation(AnimationData* animation, float deltaTimeMs);
    [[nodiscard]] float calculateEasing(float progress, AnimationType type) const;
    [[nodiscard]] float calculateCubicBezier(float t, float p1x, float p1y, float p2x, float p2y) const;

    // Memory management
    AnimationData* allocateAnimation();
    void deallocateAnimation(AnimationData* animation);
    void initializeAnimationPool();
    void cleanupCompletedAnimations();

    // Performance optimization
    void updatePerformanceMetrics();
    void adjustQualityLevel();
    void optimizeForPerformance();

    // OpenGL utilities
    void setupGLResources();
    void releaseGLResources();
    void compileShaders();

    // Utility functions
    [[nodiscard]] float clamp(float value, float min, float max) const;
    [[nodiscard]] float lerp(float a, float b, float t) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdaptiveAnimationManager)
};

} // namespace daw::ui
