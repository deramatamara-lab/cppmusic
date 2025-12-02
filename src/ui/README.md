# CppMusic Production-Grade UI System

## Overview

This directory contains the production-grade UI system for CppMusic DAW, featuring:

- **Professional Design System**: Token-driven colors, typography, and spacing
- **Smooth Animations**: 60 FPS micro-interactions with easing functions
- **HiDPI Support**: Crisp vector icons and scalable UI
- **Performance Optimized**: No allocations in paint, cached resources
- **Accessibility**: Keyboard focus indicators, tooltips with shortcuts

## Components

### Animation System (`animation/`)

Lightweight animation framework with:
- `AnimatedValue<T>`: Smooth property transitions
- `AnimatedComponent`: Timer mixin for components
- Easing functions: cubic, quadratic, elastic, back
- Zero allocations per frame

**Example:**
```cpp
AnimatedValue<float> opacity{0.5f};
opacity.setTarget(1.0f, 200, Easing::easeOutCubic);
opacity.update(deltaMs); // in timer callback
float current = opacity.getValue();
```

### Style System (`style/`)

- `LookAndFeel.hpp/cpp`: Premium dark theme with gradients, glows, and animations
- `IconManager`: Centralized SVG icon loading with HiDPI support
- `UIScaleManager`: Global UI scaling for accessibility and HiDPI displays

**Color Tokens:**
```cpp
auto& colors = lookAndFeel.getColors();
g.setColour(colors.accentPrimary);    // FL-style orange
g.setColour(colors.textSecondary);    // Dimmed text
g.setColour(colors.focus);            // Keyboard focus
```

**Typography:**
```cpp
auto font = lookAndFeel.getFont(FontType::Title, 18.0f);
auto monoFont = lookAndFeel.getFont(FontType::Monospace, 14.0f);
```

### Enhanced Components (`components/`)

- `AnimatedButton`: Buttons with hover scale and brightness animations
- `AnimatedSlider`: Sliders with smooth value interpolation and glow
- `AnimatedToggleButton`: Toggle switches with smooth state transitions
- `TooltipManager`: Tooltips with keyboard shortcut badges

### Performance Monitoring (`performance/`)

- `UIPerformanceTracker`: Track paint/layout timing
- `UI_PERF_SCOPE(label)`: RAII performance measurement

**Usage:**
```cpp
void paint(Graphics& g) override {
    UI_PERF_SCOPE("MyComponent::paint");
    // ... painting code ...
}

// Print summary:
UIPerformanceTracker::getInstance().printSummary();
```

## Design Tokens

All visual constants are defined in `assets/theme/theme.json`:

### Colors
- **Background**: `primary`, `secondary`, `tertiary`
- **Surface**: `panel`, `elevated`
- **Text**: `primary`, `secondary`, `muted`, `disabled`
- **Accent**: `primary` (orange), `secondary` (green), `tertiary` (cyan)
- **Status**: `warning`, `success`, `danger`
- **Focus**: Cyan highlight for keyboard navigation

### Typography
- **Body**: Default UI text (14pt)
- **Label**: Buttons, labels (14pt)
- **Title**: Section headers (18pt)
- **Monospace**: Time display, diagnostics (14pt)

### Spacing
- XS: 2px, SM: 4px, MD: 8px, LG: 12px, XL: 16px, XXL: 24px, XXXL: 32px

### Radius
- SM: 4px, MD: 8px, LG: 12px, XL: 16px, XXL: 22px

## Icons

SVG icons are in `assets/icons/` and loaded via `IconManager`:

```cpp
auto& iconMgr = IconManager::getInstance();

// Draw icon directly
iconMgr.drawIcon(g, IconType::Play, bounds, color);

// Get drawable
auto icon = iconMgr.getIcon(IconType::Stop, 24.0f, color);
icon->drawWithin(g, bounds, RectanglePlacement::centred, 1.0f);
```

Available icons:
- **Transport**: Play, Stop, Record, Loop, Pause
- **Tools**: Select, Draw, Slice, Eraser
- **Mixer**: Mute, Solo, Arm
- **Views**: Browser, Pattern, Playlist, Mixer, PianoRoll
- **General**: Settings, Save, Load, Export, Close, Menu
- **Edit**: Cut, Copy, Paste, Delete, Undo, Redo

## UI Scaling

Global UI scale for HiDPI and accessibility:

```cpp
auto& scaleManager = UIScaleManager::getInstance();

// Set scale (1.0 = 100%, 1.5 = 150%, 2.0 = 200%)
scaleManager.setGlobalScale(1.5f);

// Scale values
auto scaledSize = scaleManager.scale(16); // 24 at 150%
auto scaledFont = scaleManager.scaleFontSize(14.0f);

// Listen for changes
class MyComponent : public UIScaleManager::Listener {
    void uiScaleChanged(float newScale) override {
        resized();
        repaint();
    }
};
```

## Performance Guidelines

### Paint Operations
- ✅ Precompute geometry in `resized()` or constructor
- ✅ Cache gradients, paths, and drawables as members
- ✅ Use `repaint(x,y,w,h)` for partial invalidation
- ❌ No allocations in `paint()`
- ❌ No expensive math in paint loops

### Animation Performance
- ✅ Use shared timer (60 FPS) via `AnimatedComponent`
- ✅ `AnimatedValue` with inline interpolation
- ❌ No per-component timers
- ❌ No allocations per frame

### Targets
- All paint operations: < 16ms (60 FPS)
- Smooth animations: < 8ms ideal
- Monitor with: `UI_PERF_SCOPE("Label")`

## Examples

See `UIShowcase.hpp` for a complete demonstration of:
- Animated buttons with hover/press effects
- Toggle switches with smooth transitions
- Sliders with value interpolation
- Rotary knobs with glow effects
- Tooltips with keyboard shortcuts

## Testing

Build the showcase:
```bash
cmake --build build
./build/src/main/DAWProject
```

Enable performance tracking:
```cpp
UIPerformanceTracker::getInstance().setEnabled(true);
// ... use UI ...
UIPerformanceTracker::getInstance().printSummary();
```

Test at different scales:
- 100% (1920x1080)
- 150% (2560x1440)
- 200% (3840x2160)

## Future Enhancements

Planned for next iterations:
- Theme switching (light mode)
- Custom icon sets
- Animation presets
- Performance auto-tuning
- Accessibility screen reader support
