#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace daw::ui::lookandfeel
{
/**
 * @brief Design system constants
 * Centralized design tokens: colors, typography, spacing, radii, shadows.
 * DAW_DEV_RULES: single design system, no magic numbers
 */
namespace DesignSystem
{
    namespace Colors
    {
        // Base colors - 2025 refined palette
        constexpr unsigned int background           = 0xff0f0f0f;
        constexpr unsigned int surface              = 0xff1e1e1e;
        constexpr unsigned int surfaceElevated      = 0xff2a2a2a;
        constexpr unsigned int surface0             = 0xff0f0f0f;
        constexpr unsigned int surface1             = 0xff1a1a1a;
        constexpr unsigned int surface2             = 0xff1e1e1e;
        constexpr unsigned int surface3             = 0xff252525;
        constexpr unsigned int surface4             = 0xff2a2a2a;

        // Primary colors - Modern cyan
        constexpr unsigned int primary              = 0xff00d4ff;
        constexpr unsigned int primaryHover         = 0xff33e0ff;
        constexpr unsigned int primaryPressed       = 0xff00b8d9;
        constexpr unsigned int primaryLight         = 0xff66ebff;
        constexpr unsigned int primaryDark          = 0xff0099cc;

        // Secondary colors
        constexpr unsigned int secondary            = 0xff666666;
        constexpr unsigned int secondaryHover       = 0xff777777;
        constexpr unsigned int secondaryPressed     = 0xff555555;

        // Text colors
        constexpr unsigned int text                 = 0xffffffff;
        constexpr unsigned int textSoft             = 0xfff2f2f2;
        constexpr unsigned int textSecondary        = 0xffb3b3b3;
        constexpr unsigned int textTertiary         = 0xff888888;
        constexpr unsigned int textDisabled         = 0xff555555;

        // Semantic colors
        constexpr unsigned int accent               = 0xff00ffb3;
        constexpr unsigned int accentHover          = 0xff33ffc2;
        constexpr unsigned int accentPressed        = 0xff00cc8f;
        constexpr unsigned int danger               = 0xffff5555;
        constexpr unsigned int dangerHover          = 0xffff7777;
        constexpr unsigned int dangerPressed        = 0xffcc3333;
        constexpr unsigned int success              = 0xff55ff55;
        constexpr unsigned int warning              = 0xffffbb33;
        constexpr unsigned int error                = 0xffff5555;

        // UI state colors
        constexpr unsigned int hover                = 0xff2a2a2a;
        constexpr unsigned int hoverLight           = 0xff333333;
        constexpr unsigned int selected             = 0xff005577;
        constexpr unsigned int selectedHover        = 0xff006688;
        constexpr unsigned int active               = 0xff0077aa;
        constexpr unsigned int outline              = 0xff3a3a3a;
        constexpr unsigned int outlineFocus         = 0xff00d4ff;
        constexpr unsigned int divider              = 0xff2a2a2a;

        // Glassmorphism colors
        constexpr unsigned int glassBackground      = 0xcc1e1e1e;
        constexpr unsigned int glassBackgroundLight = 0x992a2a2a;
        constexpr unsigned int glassBorder          = 0x4dffffff;
        constexpr unsigned int glassShadow          = 0x80000000;
        constexpr unsigned int glassHighlight       = 0x33ffffff;

        // Meter colors
        constexpr unsigned int meterBackground      = 0xff0f0f0f;
        constexpr unsigned int meterNormal          = 0xff00ff88;
        constexpr unsigned int meterNormalStart     = 0xff00ff88;
        constexpr unsigned int meterNormalEnd       = 0xff00cc66;
        constexpr unsigned int meterWarning         = 0xffffcc00;
        constexpr unsigned int meterWarningStart    = 0xffffcc00;
        constexpr unsigned int meterWarningEnd      = 0xffff9900;
        constexpr unsigned int meterDanger          = 0xffff4444;
        constexpr unsigned int meterDangerStart     = 0xffff4444;
        constexpr unsigned int meterDangerEnd       = 0xffff0000;

        // Gradients
        constexpr unsigned int gradientPrimaryStart = 0xff00d4ff;
        constexpr unsigned int gradientPrimaryEnd   = 0xff0099cc;
        constexpr unsigned int gradientAccentStart  = 0xff00ffb3;
        constexpr unsigned int gradientAccentEnd    = 0xff00cc88;
    }

    namespace Typography
    {
        constexpr float heading1   = 24.0f;
        constexpr float heading2   = 20.0f;
        constexpr float heading3   = 18.0f;
        constexpr float body       = 14.0f;
        constexpr float bodySmall  = 12.0f;
        constexpr float caption    = 11.0f;
        constexpr float mono       = 13.0f;
    }

    namespace Spacing
    {
        constexpr int unit    = 8;
        constexpr int xsmall  = unit / 2;  // 4
        constexpr int small   = unit;      // 8
        constexpr int medium  = unit * 2;  // 16
        constexpr int large   = unit * 3;  // 24
        constexpr int xlarge  = unit * 4;  // 32
        constexpr int xxlarge = unit * 6;  // 48
    }

    namespace Radii
    {
        constexpr float none   = 0.0f;
        constexpr float small  = 2.0f;
        constexpr float medium = 4.0f;
        constexpr float large  = 8.0f;
        constexpr float xlarge = 12.0f;
    }

    namespace Shadows
    {
        struct ShadowParams { float offsetX, offsetY, blurRadius, spreadRadius, alpha; };

        // Elevations
        constexpr ShadowParams elevation0 { 0.0f, 0.0f,  0.0f,  0.0f, 0.0f };
        constexpr ShadowParams elevation1 { 0.0f, 1.0f,  2.0f,  0.0f, 0.20f };
        constexpr ShadowParams elevation2 { 0.0f, 2.0f,  4.0f,  0.0f, 0.30f };
        constexpr ShadowParams elevation3 { 0.0f, 4.0f,  8.0f,  0.0f, 0.40f };
        constexpr ShadowParams elevation4 { 0.0f, 8.0f, 16.0f,  0.0f, 0.50f };

        // Aliases
        constexpr ShadowParams small  = elevation1;
        constexpr ShadowParams medium = elevation2;
        constexpr ShadowParams large  = elevation3;

        struct ColoredShadowParams { float offsetX, offsetY, blurRadius, spreadRadius; unsigned int color; };
        constexpr ColoredShadowParams glassShadow1 { 0.0f, 2.0f,  8.0f, 0.0f, 0x40000000 };
        constexpr ColoredShadowParams glassShadow2 { 0.0f, 4.0f, 16.0f, 0.0f, 0x60000000 };
        constexpr ColoredShadowParams glassShadow3 { 0.0f, 8.0f, 24.0f, 0.0f, 0x80000000 };
    }

    namespace Animation
    {
        // Timing (ms)
        constexpr int fast   = 150;
        constexpr int normal = 250;
        constexpr int slow   = 400;

        enum class EasingType { Linear, EaseIn, EaseOut, EaseInOut };
    }

    namespace Gradients
    {
        struct GradientStop { float position; unsigned int color; };

        // Primary button
        constexpr GradientStop primaryButtonStops[] {
            { 0.00f, Colors::gradientPrimaryStart },
            { 1.00f, Colors::gradientPrimaryEnd   }
        };

        // Accent button
        constexpr GradientStop accentButtonStops[] {
            { 0.00f, Colors::gradientAccentStart },
            { 1.00f, Colors::gradientAccentEnd   }
        };

        // Meter variants
        constexpr GradientStop meterNormalStops[]  {
            { 0.00f, Colors::meterNormalStart },
            { 1.00f, Colors::meterNormalEnd   }
        };
        constexpr GradientStop meterWarningStops[] {
            { 0.00f, Colors::meterWarningStart },
            { 1.00f, Colors::meterWarningEnd   }
        };
        constexpr GradientStop meterDangerStops[]  {
            { 0.00f, Colors::meterDangerStart },
            { 1.00f, Colors::meterDangerEnd   }
        };
    }

    // ---------- Utilities (implemented in DesignSystem.cpp) -------------------

    // Convert ARGB const to juce::Colour
    [[nodiscard]] inline juce::Colour toColour(unsigned int argb) noexcept { return juce::Colour(argb); }

    // Device-aware 1-px hairline for HiDPI
    [[nodiscard]] float hairline(const juce::Component* c = nullptr) noexcept;

    // Pixel-snap a rectangle for crisp strokes
    [[nodiscard]] juce::Rectangle<float> snap(const juce::Rectangle<float>& r,
                                              const juce::Component* c = nullptr) noexcept;

    // Auto radius ("pill" on compact heights)
    [[nodiscard]] float autoRadius(float h, float base) noexcept;

    // Shadows
    void applyShadow(juce::Graphics& g, const Shadows::ShadowParams& params,
                     const juce::Rectangle<float>& bounds) noexcept;

    void applyShadow(juce::Graphics& g, const Shadows::ShadowParams& params,
                     const juce::Rectangle<float>& bounds, float cornerRadius) noexcept;

    void applyColoredShadow(juce::Graphics& g, const Shadows::ColoredShadowParams& params,
                            const juce::Rectangle<float>& bounds, float cornerRadius = Radii::large) noexcept;

    // Gradients
    void createGradientFill(juce::ColourGradient& gradient,
                            const Gradients::GradientStop* stops, size_t numStops,
                            const juce::Rectangle<float>& bounds, bool isVertical) noexcept;

    // Glass panel
    void drawGlassPanel(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                        float cornerRadius, bool elevated) noexcept;

    // Fonts
    juce::Font getHeadingFont(float size) noexcept;
    juce::Font getBodyFont(float size) noexcept;
    juce::Font getMonoFont(float size) noexcept;

    // Easing (constexpr + monotonic)
    constexpr float easeInOut(float t) noexcept { return (t < 0.5f) ? (2.0f * t * t) : (1.0f - (std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f)); }
    constexpr float easeOut(float t)   noexcept { return 1.0f - std::pow(1.0f - t, 3.0f); }
    constexpr float easeIn(float t)    noexcept { return t * t; }

    // Text with shadow
    void drawTextWithShadow(juce::Graphics& g, const juce::String& text,
                            const juce::Rectangle<float>& bounds,
                            juce::Justification justification,
                            const juce::Font& font,
                            const juce::Colour& textColor,
                            float shadowOffsetY = 1.0f,
                            float shadowAlpha = 0.30f) noexcept;

    // Focus outline
    void drawFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                       float radius, juce::Colour colour) noexcept;
}
} // namespace daw::ui::lookandfeel
