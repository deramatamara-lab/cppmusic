# Theme Tokens Specification

## Overview

The cppmusic DAW theme system uses a token-based design approach for consistent styling across all UI components. This document specifies the token structure and derivation rules.

## Token Categories

### Color Tokens

#### Semantic Colors

Base colors with semantic meaning:

| Token | Purpose | Default (Dark) |
|-------|---------|----------------|
| `primary` | Primary actions, focus states | `#3380CC` |
| `secondary` | Secondary elements | `#808099` |
| `accent` | Highlights, special elements | `#CC6633` |
| `success` | Success states, confirmations | `#33BF66` |
| `warning` | Warning states, caution | `#F2BF33` |
| `error` | Error states, destructive | `#E64040` |
| `info` | Information, hints | `#4D99E6` |

#### Background Colors

Layered backgrounds for depth:

| Token | Purpose | Default (Dark) |
|-------|---------|----------------|
| `bgPrimary` | Main window background | `#14141A` |
| `bgSecondary` | Child windows, panels | `#1A1A1F` |
| `bgTertiary` | Input fields, frames | `#1E1E24` |
| `bgElevated` | Popups, dropdowns, tooltips | `#262630` |

#### Text Colors

| Token | Purpose | Default (Dark) |
|-------|---------|----------------|
| `textPrimary` | Primary text content | `#EBEBF0` |
| `textSecondary` | Secondary text, labels | `#B3B3B8` |
| `textMuted` | Disabled, placeholder text | `#808085` |
| `textInverse` | Text on light backgrounds | `#1A1A1F` |

#### Border Colors

| Token | Purpose | Default (Dark) |
|-------|---------|----------------|
| `borderLight` | Subtle borders | `#33334080` |
| `borderMedium` | Standard borders | `#4D4D59` |
| `borderFocus` | Focus ring, active borders | `#4D80B3` |

#### DAW-Specific Colors

| Token | Purpose | Default |
|-------|---------|---------|
| `meterGreen` | Meter safe zone | `#33CC5A` |
| `meterYellow` | Meter warning zone | `#F2D933` |
| `meterRed` | Meter danger zone | `#F24040` |
| `playhead` | Playhead cursor | `#F25959` |
| `selection` | Selection highlight | `#4D80B34D` |
| `noteActive` | Active note | `#4D99E6` |
| `noteGhost` | Ghost/preview note | `#4D99E666` |
| `gridLine` | Minor grid lines | `#33334080` |
| `gridBeat` | Beat grid lines | `#4D4D59B3` |
| `gridBar` | Bar grid lines | `#6666739E` |
| `playButton` | Play button | `#33BF66` |
| `stopButton` | Stop button | `#D94D4D` |
| `recordButton` | Record button | `#F23333` |

### Spacing Tokens

Based on 8px grid system:

| Token | Value | Use Case |
|-------|-------|----------|
| `xs` | 4px | Inline spacing, tight gaps |
| `sm` | 8px | Standard small spacing |
| `md` | 16px | Default component padding |
| `lg` | 24px | Section spacing |
| `xl` | 32px | Large section gaps |
| `xxl` | 48px | Page-level spacing |

### Radius Tokens

| Token | Value | Use Case |
|-------|-------|----------|
| `none` | 0px | Sharp corners |
| `sm` | 2px | Subtle rounding |
| `md` | 4px | Standard rounding |
| `lg` | 8px | Cards, panels |
| `xl` | 12px | Large elements |
| `full` | 9999px | Pills, circles |

### Typography Tokens

| Token | Value | Use Case |
|-------|-------|----------|
| `fontSizeXs` | 10px | Tiny labels |
| `fontSizeSm` | 12px | Secondary text |
| `fontSizeMd` | 14px | Body text (default) |
| `fontSizeLg` | 18px | Headings |
| `fontSizeXl` | 24px | Large headings |
| `fontSizeXxl` | 32px | Page titles |

Line heights:

| Token | Value | Use Case |
|-------|-------|----------|
| `lineHeightTight` | 1.2 | Headings |
| `lineHeightNormal` | 1.5 | Body text |
| `lineHeightRelaxed` | 1.75 | Long-form text |

Font families:

| Token | Value | Use Case |
|-------|-------|----------|
| `fontFamilyUI` | Inter | UI text |
| `fontFamilyMono` | JetBrains Mono | Code, piano roll velocity |

### Elevation Tokens (Shadows)

| Token | Shadow | Use Case |
|-------|--------|----------|
| `none` | none | Flat elements |
| `sm` | 0 1px 2px rgba(0,0,0,0.1) | Subtle lift |
| `md` | 0 2px 4px rgba(0,0,0,0.15) | Cards |
| `lg` | 0 4px 8px rgba(0,0,0,0.2) | Floating panels |
| `xl` | 0 8px 16px rgba(0,0,0,0.25) | Modals |

### Animation Tokens

| Token | Value | Use Case |
|-------|-------|----------|
| `animFast` | 100ms | Micro-interactions |
| `animNormal` | 200ms | Standard transitions |
| `animSlow` | 400ms | Complex animations |

## Derived Values

Many colors are derived from base tokens automatically:

### Hover States

```cpp
Color hoverColor = baseColor.hover(0.1f);  // 10% lighter
```

### Disabled States

```cpp
Color disabledColor = baseColor.disabled();  // Desaturated, 60% opacity
```

### Pressed States

```cpp
Color pressedColor = baseColor.pressed(0.1f);  // 10% darker
```

### Transparency Variants

```cpp
Color transparent50 = baseColor.withAlpha(0.5f);
```

## JSON Format

Themes are stored as JSON files:

```json
{
  "name": "Custom Theme",
  "version": "1.0.0",
  "description": "A custom theme",
  "isDark": true,

  "primary": "#3380CC",
  "secondary": "#808099",
  "bgPrimary": "#14141A",
  "textPrimary": "#EBEBF0",

  "spacingXs": 4,
  "spacingSm": 8,
  "spacingMd": 16,

  "radiusSm": 2,
  "radiusMd": 4,

  "fontSizeMd": 14,

  "fontScale": 1.0,
  "dpiScale": 1.0
}
```

## Applying Themes

### Loading

```cpp
auto& theme = getGlobalThemeManager();
theme.loadFromFile("assets/themes/my_theme.json");
```

### Setting Active Theme

```cpp
theme.setTheme("Custom Theme");
```

### Accessing Tokens

```cpp
const auto& tokens = theme.getTokens();
float spacing = tokens.spacing.md;
Color primary = tokens.colors.primary;
```

### Dynamic Scaling

```cpp
// DPI scaling
theme.setDpiScale(1.5f);  // 150% scaling

// Font scaling (accessibility)
theme.setFontScale(1.25f);  // 125% font size
```

## High Contrast Mode

The high contrast theme modifies tokens for accessibility:

| Token | Default | High Contrast |
|-------|---------|---------------|
| `bgPrimary` | `#14141A` | `#000000` |
| `textPrimary` | `#EBEBF0` | `#FFFFFF` |
| `borderLight` | `#33334080` | `#808080` |
| `borderFocus` | `#4D80B3` | `#FFFF00` |

## Live Editing

The Theme Editor panel allows real-time modification:

1. Open View → Theme Editor
2. Modify token values
3. See changes immediately
4. Export diff to save changes

## Migration

Theme files include a version number. When loading older themes:

```cpp
// Register migration for version 1 → 2
theme.registerMigration(1, [](ThemeTokens& tokens, int) {
    // Add new tokens with defaults
    tokens.colors.info = Color::fromHex("#4D99E6");
    return true;
});
```

## Best Practices

### DO

- ✅ Use semantic tokens (`primary`, `error`) not raw colors
- ✅ Use spacing tokens for consistent rhythm
- ✅ Test themes at multiple DPI scales
- ✅ Provide high contrast variants
- ✅ Document custom tokens

### DON'T

- ❌ Hardcode colors in components
- ❌ Use magic number spacing values
- ❌ Ignore accessibility requirements
- ❌ Mix token systems (use one consistently)

## Reference

### Color Derivation Functions

```cpp
// Hover: Lighten color
Color hover(float amount = 0.1f) const;

// Disabled: Desaturate and reduce opacity
Color disabled() const;

// Pressed: Darken color
Color pressed(float amount = 0.1f) const;

// Mix two colors
Color mix(const Color& other, float t) const;

// Adjust alpha
Color withAlpha(float alpha) const;
```

### Parsing

```cpp
// From hex string
Color c = Color::fromHex("#4D80B3");
Color ca = Color::fromHex("#4D80B380");  // With alpha

// To hex string
std::string hex = c.toHex();  // "#4D80B3"
```
