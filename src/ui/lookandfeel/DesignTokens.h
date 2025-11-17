#pragma once

#include <juce_graphics/juce_graphics.h>

namespace daw::ui::lookandfeel
{

// Theme switch (extend later if you add system/auto)
enum class Theme { Dark, Light };

struct ColorTokens
{
    // Surfaces
    juce::Colour background{ 0xff05030b };
    juce::Colour backgroundAlt{ 0xff0c0618 };
    juce::Colour panelBackground{ 0xff120b26 };
    juce::Colour panelHighlight{ 0xff24134a };
    juce::Colour panelBorder{ juce::Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.25f) };
    juce::Colour panelShadow{ 0xff000000 };              // NEW: shadow tint for DropShadow

    // Accents
    juce::Colour accentPrimary{ 0xff8b5bff };
    juce::Colour accentSecondary{ 0xff00d0ff };
    juce::Colour accentWarning{ 0xffffc857 };

    // Text
    juce::Colour textPrimary{ 0xfff5f5ff };
    juce::Colour textSecondary{ 0xffa0a0c0 };
    juce::Colour textDisabled{ 0xff55556b };

    // Interaction/state (derived at build time in getDesignTokens)
    juce::Colour accentPrimaryHover{ 0xff8b5bff };       // derived OKLab lighten
    juce::Colour accentPrimaryActive{ 0xff8b5bff };      // derived OKLab darken
    juce::Colour onAccent{ 0xff000000 };                 // readable text on accent
    juce::Colour focusRing{ 0xff00d0ff };                // NEW: accessible focus ring
};

struct RadiusTokens
{
    float small{ 4.0f };
    float medium{ 8.0f };
    float large{ 14.0f };
};

struct SpacingTokens
{
    int xxs{ 4 };
    int xs{ 8 };
    int sm{ 12 };
    int md{ 16 };
    int lg{ 24 };
    int xl{ 32 };
};

struct ElevationTokens
{
    float panelShadowRadius{ 22.0f };
    float controlShadowRadius{ 12.0f };
    float panelShadowAlpha{ 0.35f };
    float controlShadowAlpha{ 0.25f };
};

struct TypographyTokens
{
    float smallSize{ 11.0f };
    float bodySize{ 13.0f };
    float titleSize{ 16.0f };
    float headingSize{ 20.0f };

    juce::Font small()   const { return juce::Font(juce::FontOptions(smallSize,   juce::Font::plain)); }
    juce::Font body()    const { return juce::Font(juce::FontOptions(bodySize,    juce::Font::plain)); }
    juce::Font title()   const { return juce::Font(juce::FontOptions(titleSize,   juce::Font::bold)); }
    juce::Font heading() const { return juce::Font(juce::FontOptions(headingSize, juce::Font::bold)); }
};

struct DesignTokens
{
    ColorTokens colours;
    RadiusTokens radii;
    SpacingTokens spacing;
    ElevationTokens elevation;
    TypographyTokens type;

    Theme theme{ Theme::Dark };
};

// Returns a single immutable set (thread-safe Meyers singleton). Theme may be
// switched at runtime by calling getDesignTokens(Theme::Light) before first use.
const DesignTokens& getDesignTokens(Theme theme = Theme::Dark) noexcept;

} // namespace daw::ui::lookandfeel
