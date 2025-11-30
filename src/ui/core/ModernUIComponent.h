#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <chrono>
#include <memory>
#include "../lookandfeel/DesignSystem.h"
#include "PhysicsAnimation.h"

namespace daw::ui::core
{

/**
 * ModernUIComponent - Enhanced base class for all UI components.
 *
 * Features:
 * - Multi-pass rendering system (background, content, foreground, interactive, AI visualization)
 * - Physics-based animations with spring system
 * - Performance monitoring and optimization
 * - Audio-reactive effects support
 * - Glass morphism integration
 * - Responsive design system
 * - Accessibility support
 *
 * PERFORMANCE TARGETS:
 * - 60 FPS minimum rendering
 * - <16ms paint time
 * - Memory-efficient animations
 * - Lock-free updates
 */
class ModernUIComponent : public juce::Component, public juce::Timer
{
public:
    //==============================================================================
    // MANDATORY INTERFACE - All subclasses MUST implement these

    /** Update component state from external data sources */
    virtual void update() = 0;

    /** Smooth enter animation (150-300ms duration) */
    virtual void animateIn() = 0;

    /** Smooth exit animation (150-300ms duration) */
    virtual void animateOut() = 0;

    /** Professional painting with shadows, gradients, and effects */
    virtual void paintWithShadows(juce::Graphics& g) = 0;

    //==============================================================================
    // ULTRA-SLEEK MULTI-PASS RENDERING SYSTEM

    /** Ultra-sleek rendering with multiple passes for maximum visual quality */
    void renderUltraSleek(juce::Graphics& g);

    /** Pass 1: Background with advanced glass morphism effects */
    virtual void renderBackgroundPass(juce::Graphics& g);

    /** Pass 2: Content with precise anti-aliasing and premium styling */
    virtual void renderContentPass(juce::Graphics& g);

    /** Pass 3: Foreground effects (glows, highlights, overlays) */
    virtual void renderForegroundPass(juce::Graphics& g);

    /** Pass 4: Interactive elements (ripples, particles, audio-reactive effects) */
    virtual void renderInteractivePass(juce::Graphics& g);

    /** Pass 5: AI Visualization */
    void renderAIVisualizationPass(juce::Graphics& g);

    //==============================================================================
    // MANDATORY PERFORMANCE CONSTANTS
    static constexpr float TARGET_FPS = 60.0f;
    static constexpr float MIN_FPS = 30.0f;
    static constexpr int MAX_PAINT_TIME_MS = 16;
    static constexpr float FAST_ANIMATION_MS = 150.0f;
    static constexpr float NORMAL_ANIMATION_MS = 300.0f;
    static constexpr float SLOW_ANIMATION_MS = 500.0f;
    static constexpr float MIN_CORNER_RADIUS = 4.0f;
    static constexpr float MAX_CORNER_RADIUS = 16.0f;

    //==============================================================================
    // Animation Easing Functions - PRODUCTION QUALITY
    enum class EasingType {
        Linear,
        EaseIn,
        EaseOut,
        EaseInOut,
        EaseInBack,
        EaseOutBack,
        EaseInOutBack,
        EaseInBounce,
        EaseOutBounce,
        EaseInOutBounce,
        EaseInElastic,
        EaseOutElastic,
        EaseInOutElastic
    };

    static float applyEasing(float t, EasingType easing);

    //==============================================================================
    // ULTRA-SLEEK ANIMATION SYSTEM - Physics-based animations
    UltraSleekAnimationState animationState;

    //==============================================================================
    // MANDATORY THEME INTEGRATION
    struct ThemeColors {
        juce::Colour background;
        juce::Colour surface;
        juce::Colour primary;
        juce::Colour secondary;
        juce::Colour accent;
        juce::Colour text;
        juce::Colour textSecondary;
        juce::Colour success;
        juce::Colour warning;
        juce::Colour error;
        juce::Colour shadow;
        juce::Colour glow;
    };

    //==============================================================================
    ModernUIComponent();
    virtual ~ModernUIComponent() override;

    //==============================================================================
    // JUCE Component Overrides - OPTIMIZED AND ENHANCED
    void paint(juce::Graphics& g) final override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void focusGained(juce::Component::FocusChangeType cause) override;
    void focusLost(juce::Component::FocusChangeType cause) override;

    //==============================================================================
    // MANDATORY PERFORMANCE MONITORING
    struct PerformanceMetrics {
        float averageFPS = 60.0f;
        float lastPaintTimeMs = 0.0f;
        float averagePaintTimeMs = 0.0f;
        int frameCount = 0;
        std::chrono::high_resolution_clock::time_point lastFrameTime;
    };

    const PerformanceMetrics& getPerformanceMetrics() const { return performanceMetrics; }
    void resetPerformanceMetrics();

    //==============================================================================
    // MANDATORY ACCESSIBILITY SUPPORT
    juce::String getAccessibilityTitle() const;
    juce::String getAccessibilityHelp() const;
    void setAccessibilityTitle(const juce::String& title);
    void setAccessibilityHelp(const juce::String& help);

    //==============================================================================
    // MANDATORY RESPONSIVE DESIGN
    enum class ScreenSize {
        Mobile = 480,      // 0-480px
        Tablet = 768,      // 481-768px
        Desktop = 1024,    // 769-1024px
        Large = 1440,      // 1025-1440px
        UltraWide = 1920   // 1441px+
    };

    ScreenSize getCurrentScreenSize() const;
    float getScaleFactor() const { return scaleFactor; }
    void setScaleFactor(float factor);

    //==============================================================================
    // MANDATORY SPACING SYSTEM (8px base unit)
    static constexpr int BASE_SPACING = 8;
    static constexpr int SPACING_XS = BASE_SPACING / 2;     // 4px
    static constexpr int SPACING_S = BASE_SPACING;          // 8px
    static constexpr int SPACING_M = BASE_SPACING * 2;      // 16px
    static constexpr int SPACING_L = BASE_SPACING * 3;      // 24px
    static constexpr int SPACING_XL = BASE_SPACING * 4;     // 32px
    static constexpr int SPACING_XXL = BASE_SPACING * 6;    // 48px
    static constexpr int SPACING_XXXL = BASE_SPACING * 8;   // 64px

    //==============================================================================
    // ANIMATION CONTROL
    void startAnimation(EasingType easing = EasingType::EaseInOut,
                       float duration = NORMAL_ANIMATION_MS);
    void stopAnimation();
    bool isAnimating() const { return animationState.isAnimating; }

    //==============================================================================
    // ADVANCED VISUAL EFFECTS
    void setGlowEnabled(bool enabled) { glowEnabled = enabled; }
    void setShadowEnabled(bool enabled) { shadowEnabled = enabled; }
    void setRippleEnabled(bool enabled) { rippleEnabled = enabled; }
    void setParticleEffectsEnabled(bool enabled) { particleEffectsEnabled = enabled; }

    //==============================================================================
    // AUDIO REACTIVITY
    void updateAudioLevel(float level);
    void setAudioReactive(bool reactive) { audioReactive = reactive; }

    //==============================================================================
    // Timer callback for animations
    void timerCallback() override;

protected:
    //==============================================================================
    // PROTECTED INTERFACE for subclasses

    /** Get current theme colors */
    const ThemeColors& getTheme() const { return currentTheme; }

    /** Get current animation state */
    const UltraSleekAnimationState& getAnimationState() const { return animationState; }

    /** Update animation progress (called automatically at 60fps) */
    virtual void updateAnimation(float deltaTime);

    /** Draw standard shadow behind component */
    void drawShadow(juce::Graphics& g, juce::Rectangle<float> bounds);

    /** Draw glow effect around component */
    void drawGlow(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour glowColor);

    /** Draw ripple effect from mouse interaction */
    void drawRipple(juce::Graphics& g, juce::Rectangle<float> bounds);

    /** Draw rounded rectangle with gradient */
    void drawRoundedGradient(juce::Graphics& g, juce::Rectangle<float> bounds,
                           juce::Colour topColor, juce::Colour bottomColor, float radius);

    //==============================================================================
    // PERFORMANCE OPTIMIZATION SYSTEM
    void optimizeForPerformance();
    bool shouldUseSimplifiedRendering() const;
    void updatePerformanceCache();
    void invalidateBackgroundCache() { backgroundCacheDirty = true; }

private:
    //==============================================================================
    // PRIVATE MEMBERS - OPTIMIZED FOR PERFORMANCE
    ThemeColors currentTheme;
    PerformanceMetrics performanceMetrics;

    float scaleFactor = 1.0f;
    float cornerRadius = 6.0f;
    float audioLevel = 0.0f;

    bool glowEnabled = true;
    bool shadowEnabled = true;
    bool rippleEnabled = true;
    bool particleEffectsEnabled = true;
    bool audioReactive = false;

    juce::String accessibilityTitle;
    juce::String accessibilityHelp;

    std::unique_ptr<juce::ComponentAnimator> animator;
    std::chrono::high_resolution_clock::time_point lastUpdateTime;

    // Performance optimization members
    juce::Image cachedBackground;
    bool backgroundCacheDirty = true;
    juce::Rectangle<int> dirtyRegion;
    bool repaintEntireComponent = true;

    // Animation timing
    float animationDuration = NORMAL_ANIMATION_MS;
    float animationElapsed = 0.0f;
    EasingType currentEasing = EasingType::EaseInOut;

    //==============================================================================
    // PERFORMANCE OPTIMIZATION
    void updatePerformanceMetrics();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernUIComponent)
};

} // namespace daw::ui::core

