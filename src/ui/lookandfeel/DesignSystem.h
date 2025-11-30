#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#if JUCE_MODULE_AVAILABLE_juce_animation
#include <juce_animation/juce_animation.h>
#endif
#include <atomic>
#include <array>

namespace daw::ui::lookandfeel
{
/**
 * @brief Design system constants
 * Centralized design tokens: colors, typography, spacing, radii, shadows.
 * DAW_DEV_RULES: single design system, no magic numbers
 */
namespace DesignSystem
{
    namespace detail
    {
        class ColourTokenRef
        {
        public:
            using Getter = juce::Colour (*)();
            constexpr ColourTokenRef(Getter getter = nullptr) : getterFn(getter) {}

            operator juce::Colour() const { return getterFn ? getterFn() : juce::Colours::transparentBlack; }
            operator juce::uint32() const { return getterFn ? getterFn().getARGB() : juce::Colours::transparentBlack.getARGB(); }

        private:
            Getter getterFn;
        };

        template<typename T>
        class ScalarTokenRef
        {
        public:
            using Getter = T (*)();
            constexpr ScalarTokenRef(Getter getter = nullptr) : getterFn(getter) {}

            operator T() const { return getterFn ? getterFn() : T{}; }

        private:
            Getter getterFn;
        };
    } // namespace detail

    namespace Colors
    {
        // Base colors - 2025 refined palette
        extern const detail::ColourTokenRef background;
        extern const detail::ColourTokenRef surface;
        extern const detail::ColourTokenRef surfaceElevated;
        extern const detail::ColourTokenRef surface0;
        extern const detail::ColourTokenRef surface1;
        extern const detail::ColourTokenRef surface2;
        extern const detail::ColourTokenRef surface3;
        extern const detail::ColourTokenRef surface4;

        // Primary colors - Modern cyan
        extern const detail::ColourTokenRef primary;
        extern const detail::ColourTokenRef primaryHover;
        extern const detail::ColourTokenRef primaryPressed;
        extern const detail::ColourTokenRef primaryLight;
        extern const detail::ColourTokenRef primaryDark;

        // Secondary colors
        extern const detail::ColourTokenRef secondary;
        extern const detail::ColourTokenRef secondaryHover;
        extern const detail::ColourTokenRef secondaryPressed;

        // Text colors
        extern const detail::ColourTokenRef text;
        extern const detail::ColourTokenRef textSoft;
        extern const detail::ColourTokenRef textSecondary;
        extern const detail::ColourTokenRef textTertiary;
        extern const detail::ColourTokenRef textDisabled;

        // Semantic colors
        extern const detail::ColourTokenRef accent;
        extern const detail::ColourTokenRef accentHover;
        extern const detail::ColourTokenRef accentPressed;
        extern const detail::ColourTokenRef danger;
        extern const detail::ColourTokenRef dangerHover;
        extern const detail::ColourTokenRef dangerPressed;
        extern const detail::ColourTokenRef success;
        extern const detail::ColourTokenRef warning;
        extern const detail::ColourTokenRef error;

        // UI state colors
        extern const detail::ColourTokenRef hover;
        extern const detail::ColourTokenRef hoverLight;
        extern const detail::ColourTokenRef selected;
        extern const detail::ColourTokenRef selectedHover;
        extern const detail::ColourTokenRef active;
        extern const detail::ColourTokenRef outline;
        extern const detail::ColourTokenRef outlineFocus;
        extern const detail::ColourTokenRef divider;

        // Glassmorphism colors
        extern const detail::ColourTokenRef glassBackground;
        extern const detail::ColourTokenRef glassBackgroundLight;
        extern const detail::ColourTokenRef glassBorder;
        extern const detail::ColourTokenRef glassShadow;
        extern const detail::ColourTokenRef glassHighlight;

        // Meter colors
        extern const detail::ColourTokenRef meterBackground;
        extern const detail::ColourTokenRef meterNormal;
        extern const detail::ColourTokenRef meterNormalStart;
        extern const detail::ColourTokenRef meterNormalEnd;
        extern const detail::ColourTokenRef meterWarning;
        extern const detail::ColourTokenRef meterWarningStart;
        extern const detail::ColourTokenRef meterWarningEnd;
        extern const detail::ColourTokenRef meterDanger;
        extern const detail::ColourTokenRef meterDangerStart;
        extern const detail::ColourTokenRef meterDangerEnd;

        // Gradients
        extern const detail::ColourTokenRef gradientPrimaryStart;
        extern const detail::ColourTokenRef gradientPrimaryEnd;
        extern const detail::ColourTokenRef gradientAccentStart;
        extern const detail::ColourTokenRef gradientAccentEnd;
    }

    namespace Typography
    {
        extern const detail::ScalarTokenRef<float> heading1;
        extern const detail::ScalarTokenRef<float> heading2;
        extern const detail::ScalarTokenRef<float> heading3;
        extern const detail::ScalarTokenRef<float> body;
        extern const detail::ScalarTokenRef<float> bodySmall;
        extern const detail::ScalarTokenRef<float> caption;
        extern const detail::ScalarTokenRef<float> mono;
    }

    namespace Spacing
    {
        extern const detail::ScalarTokenRef<int> unit;
        extern const detail::ScalarTokenRef<int> xsmall;
        extern const detail::ScalarTokenRef<int> small;
        extern const detail::ScalarTokenRef<int> medium;
        extern const detail::ScalarTokenRef<int> large;
        extern const detail::ScalarTokenRef<int> xlarge;
        extern const detail::ScalarTokenRef<int> xxlarge;
    }

    namespace Radii
    {
        extern const detail::ScalarTokenRef<float> none;
        extern const detail::ScalarTokenRef<float> small;
        extern const detail::ScalarTokenRef<float> medium;
        extern const detail::ScalarTokenRef<float> large;
        extern const detail::ScalarTokenRef<float> xlarge;
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
        extern const detail::ScalarTokenRef<int> fast;
        extern const detail::ScalarTokenRef<int> normal;
        extern const detail::ScalarTokenRef<int> slow;

        enum class EasingType { Linear, EaseIn, EaseOut, EaseInOut };
    }

    // DAW-specific layout constants for professional appearance
    namespace Layout
    {
        // Transport bar
        extern const detail::ScalarTokenRef<int> kTransportHeight;
        extern const detail::ScalarTokenRef<int> kStatusStripHeight;

        // Track dimensions
        extern const detail::ScalarTokenRef<int> kTrackHeight;
        extern const detail::ScalarTokenRef<int> kTrackHeaderWidth;
        extern const detail::ScalarTokenRef<int> kTrackMinimumHeight;
        extern const detail::ScalarTokenRef<int> kTrackMaximumHeight;

        // Mixer dimensions
        extern const detail::ScalarTokenRef<int> kMixerStripWidth;
        extern const detail::ScalarTokenRef<int> kMixerStripMinWidth;
        extern const detail::ScalarTokenRef<int> kMixerStripMaxWidth;
        extern const detail::ScalarTokenRef<int> kMixerFaderHeight;
        extern const detail::ScalarTokenRef<int> kMixerMeterWidth;

        // Panel dimensions
        extern const detail::ScalarTokenRef<int> kPanelMinWidth;
        extern const detail::ScalarTokenRef<int> kPanelMaxWidth;
        extern const detail::ScalarTokenRef<int> kPanelMinHeight;
        extern const detail::ScalarTokenRef<int> kPanelMaxHeight;

        // Grid and timeline
        extern const detail::ScalarTokenRef<int> kTimelineRulerHeight;
        extern const detail::ScalarTokenRef<int> kGridMinorLineWidth;
        extern const detail::ScalarTokenRef<int> kGridMajorLineWidth;
        extern const detail::ScalarTokenRef<float> kPixelsPerBeat;

        // Controls
        extern const detail::ScalarTokenRef<int> kKnobSize;
        extern const detail::ScalarTokenRef<int> kButtonHeight;
        extern const detail::ScalarTokenRef<int> kSliderHeight;
    }

    // DAW-specific color schemes for track colors, clip colors, etc.
    namespace TrackColors
    {
        // Professional track color palette (hue-based but consistent brightness/saturation)
        [[nodiscard]] juce::Colour getTrackColor(int trackIndex) noexcept;
        [[nodiscard]] juce::Colour getClipColor(int trackIndex, float velocity = 1.0f) noexcept;
        [[nodiscard]] juce::Colour getMeterColor(float level) noexcept; // level 0.0-1.0
    }

    namespace Gradients
    {
        struct GradientStop { float position; unsigned int color; };

        // Primary button
        [[nodiscard]] std::array<GradientStop, 2> primaryButtonStops() noexcept;

        // Accent button
        [[nodiscard]] std::array<GradientStop, 2> accentButtonStops() noexcept;

        // Meter variants
        [[nodiscard]] std::array<GradientStop, 2> meterNormalStops() noexcept;
        [[nodiscard]] std::array<GradientStop, 2> meterWarningStops() noexcept;
        [[nodiscard]] std::array<GradientStop, 2> meterDangerStops() noexcept;
    }

    // ---------- Higher-level semantic helpers -------------------------------

    namespace Tracks
    {
        // Deterministic track colour palette derived from accent token.
        [[nodiscard]] juce::Colour colourForIndex (int trackIndex) noexcept;
        [[nodiscard]] juce::Colour mutedColourForIndex (int trackIndex) noexcept;
    }

    namespace Meters
    {
        // Map linear gain [0, 1] → dB in a sane range for UI meters.
        [[nodiscard]] float linearToDecibels (float linear) noexcept;

        // Normalise dB value into [0, 1] for vertical meters, using a fixed
        // visible range of [-60 dB, 0 dB].
        [[nodiscard]] float normalisedFromDb (float db) noexcept;

        // Y position of 0 dB line for a vertical meter (0 = top).
        [[nodiscard]] float zeroDbLineY (const juce::Rectangle<float>& bounds) noexcept;
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

    // Semantic button styles used by drawButton/drawToggleButton helpers.
    enum class ButtonStyle { Default, Primary, Danger, Ghost, Transport };

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
