#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

namespace daw::ui::utils
{

/**
 * @brief Animation helper for smooth UI transitions
 * 
 * Provides smooth animations for component state changes.
 * Follows DAW_DEV_RULES: 60fps target, ComponentAnimator-based.
 */
class AnimationHelper
{
public:
    AnimationHelper();
    ~AnimationHelper() = default;

    /**
     * @brief Animate component bounds change
     */
    void animateBounds(juce::Component& component, juce::Rectangle<int> targetBounds, 
                      int durationMs = 200, std::function<void()> onComplete = nullptr);

    /**
     * @brief Animate component opacity
     */
    void animateOpacity(juce::Component& component, float targetAlpha, 
                       int durationMs = 200, std::function<void()> onComplete = nullptr);

    /**
     * @brief Animate component scale
     */
    void animateScale(juce::Component& component, float targetScale, 
                     int durationMs = 200, std::function<void()> onComplete = nullptr);

    /**
     * @brief Fade in component
     */
    void fadeIn(juce::Component& component, int durationMs = 200);

    /**
     * @brief Fade out component
     */
    void fadeOut(juce::Component& component, int durationMs = 200, 
                std::function<void()> onComplete = nullptr);

    /**
     * @brief Pulse animation (for highlights/notifications)
     */
    void pulse(juce::Component& component, int durationMs = 1000, int repeatCount = 3);

private:
    juce::ComponentAnimator animator;
};

/**
 * @brief Hover state manager
 */
class HoverStateManager
{
public:
    HoverStateManager() = default;
    ~HoverStateManager() = default;

    void setHovered(bool hovered) { isHovered = hovered; }
    [[nodiscard]] bool getHovered() const { return isHovered; }
    
    void setPressed(bool pressed) { isPressed = pressed; }
    [[nodiscard]] bool getPressed() const { return isPressed; }
    
    void setDisabled(bool disabled) { isDisabled = disabled; }
    [[nodiscard]] bool getDisabled() const { return isDisabled; }
    
    [[nodiscard]] bool isActive() const { return isHovered || isPressed; }

private:
    bool isHovered{false};
    bool isPressed{false};
    bool isDisabled{false};
};

} // namespace daw::ui::utils

