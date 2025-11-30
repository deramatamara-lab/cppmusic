# Enhanced UI System Usage Guide

This document describes how to use the enhanced UI techniques that have been integrated into the project.

## Overview

The enhanced UI system provides:
- **ModernUIComponent**: Base class with multi-pass rendering, physics animations, and audio reactivity
- **GlassMorphismRenderer**: Glass morphism effects for panels, buttons, sliders, and knobs
- **ProfessionalAnimationEngine**: Advanced animation system with easing, physics, and audio-reactive support
- **PremiumKnob & NeuroSlider**: Professional-grade controls with multiple styles
- **EnhancedMainLookAndFeel**: Extended LookAndFeel with glassmorphism and enhanced rendering
- **EnhancedThemeManager**: Semantic color system and theme management

## Quick Start

### Using ModernUIComponent

```cpp
#include "ui/core/ModernUIComponent.h"

class MyEnhancedPanel : public daw::ui::core::ModernUIComponent
{
public:
    MyEnhancedPanel()
    {
        setAudioReactive(true);
        setGlowEnabled(true);
        setShadowEnabled(true);
    }

    void update() override
    {
        // Update component state from external data
    }

    void animateIn() override
    {
        animationState.alphaProgress.setTarget(1.0f);
        animationState.scaleProgress.setTarget(1.0f);
        startTimerHz(60);
    }

    void animateOut() override
    {
        animationState.alphaProgress.setTarget(0.0f);
        animationState.scaleProgress.setTarget(0.0f);
        startTimerHz(60);
    }

    void paintWithShadows(juce::Graphics& g) override
    {
        // Your custom painting code here
        g.setColour(getTheme().text);
        g.drawText("Enhanced Panel", getLocalBounds(), juce::Justification::centred);
    }
};
```

### Using GlassMorphismRenderer

```cpp
#include "ui/effects/GlassMorphismRenderer.h"

void paint(juce::Graphics& g) override
{
    auto bounds = getLocalBounds().toFloat();
    
    // Render glass panel
    daw::ui::effects::GlassMorphismRenderer::renderGlassPanel(
        g, bounds, 
        daw::ui::effects::GlassMorphismRenderer::GlassStyle::Crystal
    );
    
    // Or with custom properties
    daw::ui::effects::GlassMorphismRenderer::GlassProperties props;
    props.blur = 25.0f;
    props.transparency = 0.15f;
    props.frosting = 0.4f;
    
    daw::ui::effects::GlassMorphismRenderer::renderGlassPanel(
        g, bounds, props, 
        daw::ui::effects::GlassMorphismRenderer::GlassStyle::Frosted
    );
}
```

### Using PremiumKnob

```cpp
#include "ui/components/PremiumKnob.h"

// In your component
daw::ui::components::PremiumKnob knob;
knob.setKnobStyle(daw::ui::components::PremiumKnob::KnobStyle::Modern);
knob.setRange(0.0, 100.0, 1.0);
knob.setValue(50.0);
knob.setAudioReactive(true);
knob.updateAudioLevel(0.7f); // Update from audio engine
addAndMakeVisible(knob);
```

### Using NeuroSlider

```cpp
#include "ui/components/NeuroSlider.h"

// In your component
daw::ui::components::NeuroSlider slider(
    daw::ui::components::NeuroSlider::Style::Linear
);

// Configure value mapping
daw::ui::components::NeuroSlider::ValueMapping mapping;
mapping.minValue = 20.0;
mapping.maxValue = 20000.0;
mapping.defaultValue = 1000.0;
mapping.logarithmic = true;
mapping.suffix = " Hz";
mapping.decimalPlaces = 0;
slider.setValueMapping(mapping);

// Set callbacks
slider.onValueChange = [](double value) {
    // Handle value change
};

slider.setAudioReactive(true, 1.5f);
addAndMakeVisible(slider);
```

### Using ProfessionalAnimationEngine

```cpp
#include "ui/animation/ProfessionalAnimationEngine.h"

// Create engine instance (typically as member variable)
daw::ui::animation::ProfessionalAnimationEngine animationEngine;
animationEngine.initialize();

// Create animation
daw::ui::animation::ProfessionalAnimationEngine::AnimationConfig config;
config.type = daw::ui::animation::ProfessionalAnimationEngine::AnimationType::Position;
config.easing = daw::ui::animation::ProfessionalAnimationEngine::EasingType::EaseInOut;
config.duration = 0.5f; // 500ms

daw::ui::animation::ProfessionalAnimationEngine::AnimationTarget target;
target.component = myComponent;
target.startPosition = juce::Point<float>(0, 0);
target.endPosition = juce::Point<float>(100, 100);

auto animId = animationEngine.createAnimation(config, target);
animationEngine.startAnimation(animId);

// Update engine each frame
animationEngine.update();
```

### Using EnhancedMainLookAndFeel

```cpp
#include "ui/lookandfeel/EnhancedMainLookAndFeel.h"

// Set as global LookAndFeel
auto enhancedLookAndFeel = std::make_unique<daw::ui::lookandfeel::EnhancedMainLookAndFeel>(
    daw::ui::lookandfeel::Theme::Dark
);
juce::LookAndFeel::setDefaultLookAndFeel(enhancedLookAndFeel.get());

// Use enhanced rendering methods
enhancedLookAndFeel->drawEnhancedPanel(g, bounds, true, true); // highlighted, glassmorphism
enhancedLookAndFeel->drawProfessionalMeter(g, bounds, 0.75f, 0.9f, true); // level, peak, vertical
```

### Using EnhancedThemeManager

```cpp
#include "ui/lookandfeel/EnhancedThemeManager.h"

daw::ui::lookandfeel::EnhancedThemeManager themeManager;

// Get semantic colors
auto bgColor = themeManager.getColor(
    daw::ui::lookandfeel::EnhancedThemeManager::ColorToken::BackgroundPrimary
);

auto accentColor = themeManager.getColor(
    daw::ui::lookandfeel::EnhancedThemeManager::ColorToken::AccentPrimary
);

// Get spacing
float spacing = themeManager.getSpacing(
    daw::ui::lookandfeel::EnhancedThemeManager::SpacingToken::Spacing4
); // Returns 16.0f

// Get typography
auto font = themeManager.getHeadingFont(24.0f);
auto bodyFont = themeManager.getFont(14.0f);
auto monoFont = themeManager.getMonoFont(13.0f);

// Apply shadows
themeManager.applyShadow(g, 2, bounds, 8.0f); // elevation 2, corner radius 8
```

## Integration with Existing Components

To enhance existing components:

1. **Inherit from ModernUIComponent** instead of `juce::Component`:
   ```cpp
   class MyPanel : public daw::ui::core::ModernUIComponent
   ```

2. **Use glassmorphism** in paint methods:
   ```cpp
   daw::ui::effects::GlassMorphismRenderer::renderGlassPanel(g, bounds);
   ```

3. **Replace standard sliders/knobs** with PremiumKnob and NeuroSlider

4. **Apply enhanced LookAndFeel** globally or per-component

## Performance Considerations

- ModernUIComponent automatically optimizes rendering when performance drops
- Glassmorphism effects can be disabled for better performance
- Audio-reactive effects should only be enabled when needed
- Use `shouldUseSimplifiedRendering()` to check if optimizations are active

## Design System Integration

All components use the centralized `DesignSystem`:
- Colors: `daw::ui::lookandfeel::DesignSystem::Colors`
- Spacing: `daw::ui::lookandfeel::DesignSystem::Spacing`
- Typography: `daw::ui::lookandfeel::DesignSystem::Typography`
- Shadows: `daw::ui::lookandfeel::DesignSystem::Shadows`

No magic numbers - everything uses design tokens!

