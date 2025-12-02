# UI Animation System

## Overview

The CppMusic DAW uses a lightweight, performance-optimized animation system to provide smooth micro-interactions and polished UI feel. This system is designed with the following principles:

- **Zero allocations per frame**: All animations use pre-allocated state
- **Shared timer**: Single 60 FPS timer for all animations
- **Easing functions**: Natural motion with cubic, quadratic, and elastic easing
- **Type-safe**: Template-based for any numeric type or JUCE color

## Quick Start

### Basic Animation

```cpp
#include "animation/Animation.hpp"

class MyComponent : public juce::Component, 
                    public cppmusic::ui::animation::AnimatedComponent {
public:
    void mouseEnter(const juce::MouseEvent&) override {
        opacity_.setTarget(1.0f, 200, Easing::easeOutCubic);
        startAnimation([this](float delta) {
            bool isAnimating = opacity_.update(delta);
            repaint();
            return isAnimating;
        });
    }
    
    void paint(juce::Graphics& g) override {
        g.setOpacity(opacity_.getValue());
        // ... painting code ...
    }
    
private:
    AnimatedValue<float> opacity_{0.5f};
};
```

### Button Hover Animation

```cpp
class AnimatedButton : public juce::TextButton,
                       public cppmusic::ui::animation::AnimatedComponent {
public:
    AnimatedButton() : scale_(1.0f) {}
    
    void mouseEnter(const juce::MouseEvent&) override {
        scale_.setTarget(1.05f, 150, Easing::easeOutBack);
        startAnimation([this](float delta) {
            scale_.update(delta);
            repaint();
            return scale_.isAnimating();
        });
    }
    
    void mouseExit(const juce::MouseEvent&) override {
        scale_.setTarget(1.0f, 150, Easing::easeOutCubic);
        startAnimation([this](float delta) {
            scale_.update(delta);
            repaint();
            return scale_.isAnimating();
        });
    }
    
    void paint(juce::Graphics& g) override {
        auto transform = juce::AffineTransform::scale(scale_.getValue());
        g.addTransform(transform);
        TextButton::paint(g);
    }
    
private:
    AnimatedValue<float> scale_;
};
```

## Easing Functions

Available easing functions in `cppmusic::ui::animation::Easing`:

- `linear(t)` - Constant speed
- `easeInQuad(t)` / `easeOutQuad(t)` / `easeInOutQuad(t)` - Quadratic
- `easeInCubic(t)` / `easeOutCubic(t)` / `easeInOutCubic(t)` - Cubic (recommended)
- `easeInQuart(t)` / `easeOutQuart(t)` / `easeInOutQuart(t)` - Quartic
- `easeOutElastic(t)` - Bouncy spring effect
- `easeOutBack(t)` - Slight overshoot

## AnimatedValue API

```cpp
AnimatedValue<T> value(initialValue);

// Set target with duration and easing
value.setTarget(target, durationMs, easingFunc);

// Update in timer callback
bool isAnimating = value.update(deltaMs);

// Get current value
T current = value.getValue();

// Set immediately without animation
value.setValue(newValue);

// Check if animating
if (value.isAnimating()) { /* ... */ }
```

## Performance Best Practices

1. **Use AnimatedComponent mixin**: Provides automatic timer management
2. **Share timers**: Don't create per-component timers
3. **Minimize repaints**: Only repaint the animated area
4. **Cache transforms**: Pre-calculate transforms in update, not paint
5. **Profile with UI_PERF_SCOPE**: Monitor animation performance

## Examples in Codebase

See these components for reference implementations:

- `src/ui/style/LookAndFeel.cpp` - Button and slider animations
- Enhanced components with hover/press animations
- Smooth playhead movement in transport

## Performance Monitoring

Enable UI performance tracking in debug builds:

```cpp
UIPerformanceTracker::getInstance().setEnabled(true);

// In paint():
UI_PERF_SCOPE("MyComponent::paint");

// Print summary:
UIPerformanceTracker::getInstance().printSummary();
```

Target: All paint operations < 16ms (60 FPS), ideally < 8ms.
