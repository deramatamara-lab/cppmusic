# UI Component Guide

## Production-Grade Component Usage

This guide demonstrates how to use the new production-grade UI components in CppMusic DAW.

## Table of Contents

1. [Animated Buttons](#animated-buttons)
2. [Animated Sliders](#animated-sliders)
3. [Tooltips](#tooltips)
4. [Icons](#icons)
5. [UI Scaling](#ui-scaling)
6. [Performance Monitoring](#performance-monitoring)
7. [Custom Animations](#custom-animations)

## Animated Buttons

### Basic Animated Button

```cpp
#include "components/AnimatedButton.hpp"

class MyComponent : public juce::Component {
public:
    MyComponent() {
        playButton_.setButtonText("Play");
        playButton_.onClick = [this] { handlePlay(); };
        addAndMakeVisible(playButton_);
    }
    
private:
    cppmusic::ui::components::AnimatedButton playButton_;
};
```

Features:
- Hover: Scales to 105% with subtle bounce (easeOutBack)
- Press: Scales to 95% with quick easing
- Brightness: Increases 10% on hover
- Zero configuration needed

### Animated Toggle Button

```cpp
#include "components/AnimatedButton.hpp"

AnimatedToggleButton muteButton_{"Mute"};
muteButton_.onClick = [this] {
    auto isMuted = muteButton_.getToggleState();
    processor_.setMute(isMuted);
};
```

Features:
- Smooth 200ms color transition
- Animated knob position
- Professional switch appearance

## Animated Sliders

### Linear Slider with Animation

```cpp
#include "components/AnimatedSlider.hpp"

AnimatedSlider volumeSlider_;
volumeSlider_.setRange(-60.0, 6.0);
volumeSlider_.setValue(0.0);
volumeSlider_.onValueChange = [this] {
    processor_.setVolume(volumeSlider_.getValue());
};
```

Features:
- Smooth value interpolation (150ms)
- Hover glow effect
- Visual feedback during drag

### Rotary Knob

```cpp
AnimatedKnob filterCutoff_;
filterCutoff_.setRange(20.0, 20000.0);
filterCutoff_.setSkewFactorFromMidPoint(1000.0);
filterCutoff_.setDoubleClickReturnValue(true, 1000.0);
filterCutoff_.onValueChange = [this] {
    processor_.setCutoff(filterCutoff_.getValue());
};
```

Features:
- Double-click to reset to default
- Animated value changes
- Professional appearance from LookAndFeel

### Programmatic Animation

```cpp
// Animate to a value over time
volumeSlider_.setValueAnimated(-6.0, 500); // 500ms duration

// Quick reset
volumeSlider_.resetToDefault();
```

## Tooltips

### Basic Tooltip

```cpp
playButton_.setTooltip("Start playback");
```

### Tooltip with Keyboard Shortcut

```cpp
#include "components/TooltipManager.hpp"

class MyButton : public AnimatedButton, 
                 public TooltipManager {
public:
    MyButton() {
        setTooltipWithShortcut("Start playback", "Space");
    }
};
```

### Custom Tooltip Window

```cpp
#include "components/TooltipManager.hpp"

AnimatedTooltipWindow tooltipWindow_;

// Show tooltip
tooltipWindow_.displayTip(
    mousePosition,
    "Drag to adjust tempo",
    "Shift+Drag"  // Keyboard shortcut
);

// Hide tooltip
tooltipWindow_.hide();
```

## Icons

### Using IconManager

```cpp
#include "style/IconManager.hpp"

void paint(Graphics& g) override {
    auto& iconMgr = IconManager::getInstance();
    
    // Draw icon directly
    auto iconBounds = getLocalBounds().removeFromLeft(24).toFloat();
    iconMgr.drawIcon(g, IconType::Play, iconBounds, 
                    Colour(0xFFE8ECF7));
}
```

### Getting Drawable Icons

```cpp
auto playIcon = iconMgr.getIcon(IconType::Play, 24.0f, 
                                Colour(0xFFFFA726));
playIcon->drawWithin(g, bounds, RectanglePlacement::centred, 1.0f);
```

### Available Icons

Transport: `Play`, `Stop`, `Record`, `Loop`, `Pause`  
Tools: `Select`, `Draw`, `Slice`, `Eraser`  
Mixer: `Mute`, `Solo`, `Arm`  
General: `Settings`, `Save`, `Load`, `Delete`, `Menu`, `Close`  
Edit: `Undo`, `Redo`

## UI Scaling

### Responding to Scale Changes

```cpp
#include "style/UIScaleManager.hpp"

class MyComponent : public Component,
                    public UIScaleManager::Listener {
public:
    MyComponent() {
        UIScaleManager::getInstance().addListener(this);
    }
    
    ~MyComponent() override {
        UIScaleManager::getInstance().removeListener(this);
    }
    
    void uiScaleChanged(float newScale) override {
        // Respond to scale changes
        resized();
        repaint();
    }
    
    void paint(Graphics& g) override {
        auto& scaleManager = UIScaleManager::getInstance();
        
        // Scale values
        auto fontSize = scaleManager.scaleFontSize(14.0f);
        auto padding = scaleManager.scale(8);
        
        g.setFont(fontSize);
        // ... rest of painting
    }
};
```

### Manual Scaling

```cpp
auto& scaleManager = UIScaleManager::getInstance();

// Set scale
scaleManager.setGlobalScale(1.5f); // 150%

// Cycle through presets (1.0, 1.25, 1.5, 1.75, 2.0)
scaleManager.cycleScale();

// Get recommended scale for current display
auto recommended = scaleManager.getRecommendedScale();
```

## Performance Monitoring

### Tracking Component Performance

```cpp
#include "performance/UIPerformance.hpp"

void paint(Graphics& g) override {
    UI_PERF_SCOPE("MyComponent::paint");
    
    // ... painting code ...
}

void resized() override {
    UI_PERF_SCOPE("MyComponent::resized");
    
    // ... layout code ...
}
```

### Viewing Performance Metrics

```cpp
// Enable tracking
UIPerformanceTracker::getInstance().setEnabled(true);

// ... use UI for a while ...

// Print summary to debug console
UIPerformanceTracker::getInstance().printSummary();

// Reset metrics
UIPerformanceTracker::getInstance().reset();
```

Output example:
```
=== UI Performance Summary ===
Label                              Count    Min(ms)  Avg(ms)  Max(ms)
-----------------------------------------------------------------------
Playlist::paint                      342       2.14      3.52      8.41
PianoRoll::paint                     189       3.21      5.67     12.33
Mixer::paint                         456       1.02      1.84      4.21
===============================
```

## Custom Animations

### Simple Property Animation

```cpp
#include "animation/Animation.hpp"

class MyComponent : public Component,
                    public AnimatedComponent {
public:
    void mouseEnter(const MouseEvent&) override {
        // Animate opacity
        opacity_.setTarget(1.0f, 200, Easing::easeOutCubic);
        
        startAnimation([this](float delta) {
            bool isAnimating = opacity_.update(delta);
            if (isAnimating) {
                repaint();
            }
            return isAnimating;
        });
    }
    
    void paint(Graphics& g) override {
        g.setOpacity(opacity_.getValue());
        // ... painting
    }
    
private:
    AnimatedValue<float> opacity_{0.5f};
};
```

### Multiple Animated Properties

```cpp
class AnimatedPanel : public Component,
                      public AnimatedComponent {
public:
    void show() {
        opacity_.setTarget(1.0f, 300, Easing::easeOutCubic);
        slideY_.setTarget(0.0f, 300, Easing::easeOutBack);
        
        startAnimation([this](float delta) {
            bool o = opacity_.update(delta);
            bool y = slideY_.update(delta);
            
            if (o || y) {
                repaint();
            }
            return o || y;
        });
        
        setVisible(true);
    }
    
    void hide() {
        opacity_.setTarget(0.0f, 200, Easing::easeOutCubic);
        slideY_.setTarget(-50.0f, 200, Easing::easeInCubic);
        
        startAnimation([this](float delta) {
            bool o = opacity_.update(delta);
            bool y = slideY_.update(delta);
            
            if (o || y) {
                repaint();
            } else {
                setVisible(false);
            }
            return o || y;
        });
    }
    
    void paint(Graphics& g) override {
        g.setOpacity(opacity_.getValue());
        auto transform = AffineTransform::translation(0, slideY_.getValue());
        g.addTransform(transform);
        
        // ... painting
    }
    
private:
    AnimatedValue<float> opacity_{0.0f};
    AnimatedValue<float> slideY_{-50.0f};
};
```

### Available Easing Functions

```cpp
using namespace cppmusic::ui::animation::Easing;

// Linear
linear(t)

// Quadratic
easeInQuad(t), easeOutQuad(t), easeInOutQuad(t)

// Cubic (recommended for most animations)
easeInCubic(t), easeOutCubic(t), easeInOutCubic(t)

// Quartic (more pronounced)
easeInQuart(t), easeOutQuart(t), easeInOutQuart(t)

// Special effects
easeOutElastic(t)  // Bouncy spring effect
easeOutBack(t)     // Slight overshoot
```

## Best Practices

### Performance
- ✅ Use `UI_PERF_SCOPE` in debug builds
- ✅ Cache complex graphics in member variables
- ✅ Use `repaint(x,y,w,h)` for partial updates
- ❌ No allocations in `paint()`
- ❌ No heavy math in paint loops

### Animation
- ✅ Use `AnimatedComponent` mixin
- ✅ Prefer `easeOutCubic` for most animations
- ✅ Keep durations 100-300ms for responsiveness
- ❌ No per-component timers
- ❌ No allocations per frame

### Accessibility
- ✅ Add tooltips with keyboard shortcuts
- ✅ Use focus color from ColorPalette
- ✅ Support UI scaling
- ✅ Test at 150% and 200% scale

### Testing
- Build and run UIShowcase component
- Test at different resolutions
- Monitor performance with UIPerformanceTracker
- Verify smooth 60 FPS animations
