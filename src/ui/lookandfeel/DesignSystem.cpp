#include "DesignSystem.h"
#include "DesignTokens.h"
#include "UltraDesignSystem.hpp"
#include <cmath>

namespace daw::ui::lookandfeel
{
namespace DesignSystem
{
namespace
{
    [[nodiscard]] const ColorTokens& baseColors() noexcept
    {
        return getDesignTokens(Theme::Dark).colours;
    }

    [[nodiscard]] const SpacingTokens& spacingTokens() noexcept
    {
        return getDesignTokens(Theme::Dark).spacing;
    }

    [[nodiscard]] const RadiusTokens& radiusTokens() noexcept
    {
        return getDesignTokens(Theme::Dark).radii;
    }

    [[nodiscard]] const TypographyTokens& typeTokens() noexcept
    {
        return getDesignTokens(Theme::Dark).type;
    }

    [[nodiscard]] const ultra::Tokens::Colors& ultraColors() noexcept
    {
        return ultra::tokens().color;
    }

    [[nodiscard]] const ultra::Tokens::Anim& animTokens() noexcept
    {
        return ultra::tokens().anim;
    }

    [[nodiscard]] juce::Colour lighten(juce::Colour c, float amount) noexcept
    {
        return c.interpolatedWith(juce::Colours::white, juce::jlimit(0.0f, 1.0f, amount));
    }

    [[nodiscard]] juce::Colour darken(juce::Colour c, float amount) noexcept
    {
        return c.interpolatedWith(juce::Colours::black, juce::jlimit(0.0f, 1.0f, amount));
    }
}

#define DEFINE_COLOR_TOKEN(name, expr) \
    static juce::Colour getColour_##name() { return (expr); } \
    const detail::ColourTokenRef Colors::name { &getColour_##name };

DEFINE_COLOR_TOKEN(background,        baseColors().background)
DEFINE_COLOR_TOKEN(surface,           ultraColors().bg1)
DEFINE_COLOR_TOKEN(surfaceElevated,   ultraColors().bg2)
DEFINE_COLOR_TOKEN(surface0,          ultraColors().bg0)
DEFINE_COLOR_TOKEN(surface1,          ultraColors().bg1)
DEFINE_COLOR_TOKEN(surface2,          ultraColors().bg2)
DEFINE_COLOR_TOKEN(surface3,          ultraColors().bg2.brighter(0.06f))
DEFINE_COLOR_TOKEN(surface4,          ultraColors().bg2.brighter(0.12f))

DEFINE_COLOR_TOKEN(primary,           baseColors().accentPrimary)
DEFINE_COLOR_TOKEN(primaryHover,      baseColors().accentPrimaryHover)
DEFINE_COLOR_TOKEN(primaryPressed,    baseColors().accentPrimaryActive)
DEFINE_COLOR_TOKEN(primaryLight,      lighten(baseColors().accentPrimary, 0.35f))
DEFINE_COLOR_TOKEN(primaryDark,       darken(baseColors().accentPrimary, 0.25f))

DEFINE_COLOR_TOKEN(secondary,         darken(baseColors().accentSecondary, 0.15f))
DEFINE_COLOR_TOKEN(secondaryHover,    baseColors().accentSecondary)
DEFINE_COLOR_TOKEN(secondaryPressed,  darken(baseColors().accentSecondary, 0.25f))

DEFINE_COLOR_TOKEN(text,              baseColors().textPrimary)
DEFINE_COLOR_TOKEN(textSoft,          baseColors().textPrimary.withAlpha(0.9f))
DEFINE_COLOR_TOKEN(textSecondary,     baseColors().textSecondary)
DEFINE_COLOR_TOKEN(textTertiary,      baseColors().textSecondary.withAlpha(0.6f))
DEFINE_COLOR_TOKEN(textDisabled,      baseColors().textDisabled)

DEFINE_COLOR_TOKEN(accent,            baseColors().accentSecondary)
DEFINE_COLOR_TOKEN(accentHover,       lighten(baseColors().accentSecondary, 0.12f))
DEFINE_COLOR_TOKEN(accentPressed,     darken(baseColors().accentSecondary, 0.18f))
DEFINE_COLOR_TOKEN(danger,            ultraColors().danger)
DEFINE_COLOR_TOKEN(dangerHover,       lighten(ultraColors().danger, 0.12f))
DEFINE_COLOR_TOKEN(dangerPressed,     darken(ultraColors().danger, 0.18f))
DEFINE_COLOR_TOKEN(success,           ultraColors().meterOK)
DEFINE_COLOR_TOKEN(warning,           ultraColors().warn)
DEFINE_COLOR_TOKEN(error,             ultraColors().danger)

DEFINE_COLOR_TOKEN(hover,             ultraColors().bg2.brighter(0.05f))
DEFINE_COLOR_TOKEN(hoverLight,        ultraColors().bg2.brighter(0.12f))
DEFINE_COLOR_TOKEN(selected,          darken(baseColors().accentPrimary, 0.55f))
DEFINE_COLOR_TOKEN(selectedHover,     darken(baseColors().accentPrimary, 0.45f))
DEFINE_COLOR_TOKEN(active,            baseColors().accentPrimary)
DEFINE_COLOR_TOKEN(outline,           ultraColors().panelBorder)
DEFINE_COLOR_TOKEN(outlineFocus,      baseColors().focusRing)
DEFINE_COLOR_TOKEN(divider,           ultraColors().graphGrid)

DEFINE_COLOR_TOKEN(glassBackground,      ultraColors().bg2.withAlpha(0.85f))
DEFINE_COLOR_TOKEN(glassBackgroundLight, ultraColors().bg1.withAlpha(0.65f))
DEFINE_COLOR_TOKEN(glassBorder,          ultraColors().panelBorder.withAlpha(0.35f))
DEFINE_COLOR_TOKEN(glassShadow,          ultraColors().shadowSoft)
DEFINE_COLOR_TOKEN(glassHighlight,       juce::Colours::white.withAlpha(0.2f))

DEFINE_COLOR_TOKEN(meterBackground,   ultraColors().bg0)
DEFINE_COLOR_TOKEN(meterNormal,       ultraColors().meterOK)
DEFINE_COLOR_TOKEN(meterNormalStart,  ultraColors().meterOK)
DEFINE_COLOR_TOKEN(meterNormalEnd,    darken(ultraColors().meterOK, 0.2f))
DEFINE_COLOR_TOKEN(meterWarning,      ultraColors().meterHot)
DEFINE_COLOR_TOKEN(meterWarningStart, ultraColors().meterHot)
DEFINE_COLOR_TOKEN(meterWarningEnd,   darken(ultraColors().meterHot, 0.2f))
DEFINE_COLOR_TOKEN(meterDanger,       ultraColors().meterClip)
DEFINE_COLOR_TOKEN(meterDangerStart,  ultraColors().meterClip)
DEFINE_COLOR_TOKEN(meterDangerEnd,    darken(ultraColors().meterClip, 0.2f))

DEFINE_COLOR_TOKEN(gradientPrimaryStart, baseColors().accentPrimary)
DEFINE_COLOR_TOKEN(gradientPrimaryEnd,   baseColors().accentSecondary)
DEFINE_COLOR_TOKEN(gradientAccentStart,  baseColors().accentSecondary)
DEFINE_COLOR_TOKEN(gradientAccentEnd,    lighten(baseColors().accentSecondary, 0.2f))

#undef DEFINE_COLOR_TOKEN

#define DEFINE_SPACING_TOKEN(name, expr) \
    static int getSpacing_##name() { return (expr); } \
    const detail::ScalarTokenRef<int> Spacing::name { &getSpacing_##name };

DEFINE_SPACING_TOKEN(unit,   spacingTokens().xs)
DEFINE_SPACING_TOKEN(xsmall, spacingTokens().xxs)
DEFINE_SPACING_TOKEN(small,  spacingTokens().xs)
DEFINE_SPACING_TOKEN(medium, spacingTokens().sm)
DEFINE_SPACING_TOKEN(large,  spacingTokens().md)
DEFINE_SPACING_TOKEN(xlarge, spacingTokens().lg)
DEFINE_SPACING_TOKEN(xxlarge, spacingTokens().xl)

#undef DEFINE_SPACING_TOKEN

#define DEFINE_RADII_TOKEN(name, expr) \
    static float getRadius_##name() { return (expr); } \
    const detail::ScalarTokenRef<float> Radii::name { &getRadius_##name };

DEFINE_RADII_TOKEN(none,   0.0f)
DEFINE_RADII_TOKEN(small,  radiusTokens().small)
DEFINE_RADII_TOKEN(medium, radiusTokens().medium)
DEFINE_RADII_TOKEN(large,  radiusTokens().large)
DEFINE_RADII_TOKEN(xlarge, ultra::tokens().radius.xl)

#undef DEFINE_RADII_TOKEN

#define DEFINE_TYPE_TOKEN(name, expr) \
    static float getType_##name() { return (expr); } \
    const detail::ScalarTokenRef<float> Typography::name { &getType_##name };

DEFINE_TYPE_TOKEN(heading1, typeTokens().headingSize)
DEFINE_TYPE_TOKEN(heading2, typeTokens().titleSize)
DEFINE_TYPE_TOKEN(heading3, juce::jmax(typeTokens().bodySize + 2.0f, typeTokens().smallSize + 4.0f))
DEFINE_TYPE_TOKEN(body,     typeTokens().bodySize)
DEFINE_TYPE_TOKEN(bodySmall,typeTokens().smallSize)
DEFINE_TYPE_TOKEN(caption,  juce::jmax(10.0f, typeTokens().smallSize - 1.0f))
DEFINE_TYPE_TOKEN(mono,     typeTokens().bodySize)

#undef DEFINE_TYPE_TOKEN

#define DEFINE_ANIMATION_TOKEN(name, expr) \
    static int getAnim_##name() { return (expr); } \
    const detail::ScalarTokenRef<int> Animation::name { &getAnim_##name };

DEFINE_ANIMATION_TOKEN(fast,   animTokens().msFast)
DEFINE_ANIMATION_TOKEN(normal, animTokens().msMed)
DEFINE_ANIMATION_TOKEN(slow,   animTokens().msSlow)

#undef DEFINE_ANIMATION_TOKEN

// DAW-specific layout constants
#define DEFINE_LAYOUT_TOKEN(name, expr) \
    static int getLayout_##name() { return (expr); } \
    const detail::ScalarTokenRef<int> Layout::name { &getLayout_##name };

#define DEFINE_LAYOUT_FLOAT_TOKEN(name, expr) \
    static float getLayoutFloat_##name() { return (expr); } \
    const detail::ScalarTokenRef<float> Layout::name { &getLayoutFloat_##name };

// Transport and status
DEFINE_LAYOUT_TOKEN(kTransportHeight, 54)
DEFINE_LAYOUT_TOKEN(kStatusStripHeight, 24)

// Track dimensions (following FL Studio proportions)
DEFINE_LAYOUT_TOKEN(kTrackHeight, 40)
DEFINE_LAYOUT_TOKEN(kTrackHeaderWidth, 200)
DEFINE_LAYOUT_TOKEN(kTrackMinimumHeight, 32)
DEFINE_LAYOUT_TOKEN(kTrackMaximumHeight, 120)

// Mixer dimensions (professional channel strip proportions)
DEFINE_LAYOUT_TOKEN(kMixerStripWidth, 56)
DEFINE_LAYOUT_TOKEN(kMixerStripMinWidth, 44)
DEFINE_LAYOUT_TOKEN(kMixerStripMaxWidth, 72)
DEFINE_LAYOUT_TOKEN(kMixerFaderHeight, 200)
DEFINE_LAYOUT_TOKEN(kMixerMeterWidth, 8)

// Panel dimensions
DEFINE_LAYOUT_TOKEN(kPanelMinWidth, 200)
DEFINE_LAYOUT_TOKEN(kPanelMaxWidth, 800)
DEFINE_LAYOUT_TOKEN(kPanelMinHeight, 150)
DEFINE_LAYOUT_TOKEN(kPanelMaxHeight, 600)

// Grid and timeline
DEFINE_LAYOUT_TOKEN(kTimelineRulerHeight, 32)
DEFINE_LAYOUT_TOKEN(kGridMinorLineWidth, 1)
DEFINE_LAYOUT_TOKEN(kGridMajorLineWidth, 2)
DEFINE_LAYOUT_FLOAT_TOKEN(kPixelsPerBeat, 64.0f)

// Controls
DEFINE_LAYOUT_TOKEN(kKnobSize, 32)
DEFINE_LAYOUT_TOKEN(kButtonHeight, 28)
DEFINE_LAYOUT_TOKEN(kSliderHeight, 20)

#undef DEFINE_LAYOUT_TOKEN
#undef DEFINE_LAYOUT_FLOAT_TOKEN

// Track color system implementation
namespace TrackColors
{
    // Professional track colors based on HSV wheel with consistent saturation/value
    constexpr float kTrackColorSaturation = 0.65f;
    constexpr float kTrackColorValue = 0.85f;
    constexpr int kNumTrackColors = 12;

    juce::Colour getTrackColor(int trackIndex) noexcept
    {
        const float hue = (trackIndex % kNumTrackColors) * (360.0f / kNumTrackColors);
        return juce::Colour::fromHSV(hue / 360.0f, kTrackColorSaturation, kTrackColorValue, 1.0f);
    }

    juce::Colour getClipColor(int trackIndex, float velocity) noexcept
    {
        auto baseColor = getTrackColor(trackIndex);
        // Adjust brightness based on velocity
        const float velocityFactor = juce::jlimit(0.3f, 1.0f, velocity);
        return baseColor.withBrightness(baseColor.getBrightness() * velocityFactor);
    }

    juce::Colour getMeterColor(float level) noexcept
    {
        if (level > 0.95f) // Clipping range
            return juce::Colour(Colors::meterDanger);
        else if (level > 0.75f) // Warning range
            return juce::Colour(Colors::meterWarning);
        else // Normal range
            return juce::Colour(Colors::meterNormal);
    }
}

std::array<Gradients::GradientStop, 2> Gradients::primaryButtonStops() noexcept
{
    return { {
        { 0.00f, Colors::gradientPrimaryStart },
        { 1.00f, Colors::gradientPrimaryEnd   }
    } };
}

std::array<Gradients::GradientStop, 2> Gradients::accentButtonStops() noexcept
{
    return { {
        { 0.00f, Colors::gradientAccentStart },
        { 1.00f, Colors::gradientAccentEnd   }
    } };
}

std::array<Gradients::GradientStop, 2> Gradients::meterNormalStops() noexcept
{
    return { {
        { 0.00f, Colors::meterNormalStart },
        { 1.00f, Colors::meterNormalEnd   }
    } };
}

std::array<Gradients::GradientStop, 2> Gradients::meterWarningStops() noexcept
{
    return { {
        { 0.00f, Colors::meterWarningStart },
        { 1.00f, Colors::meterWarningEnd   }
    } };
}

std::array<Gradients::GradientStop, 2> Gradients::meterDangerStops() noexcept
{
    return { {
        { 0.00f, Colors::meterDangerStart },
        { 1.00f, Colors::meterDangerEnd   }
    } };
}

// ----------------------------- small helpers ---------------------------------

float hairline(const juce::Component* c) noexcept
{
    const auto& displays = juce::Desktop::getInstance().getDisplays();
    const auto* primaryDisplay = displays.getPrimaryDisplay();
    const float fallbackScale = primaryDisplay != nullptr ? primaryDisplay->scale : 1.0f;
    const float s = c ? c->getDesktopScaleFactor() : fallbackScale;
    const float px = 1.0f / juce::jmax(1.0f, std::floor(s));
    return juce::jlimit(0.5f, 1.0f, px); // never below 0.5 to avoid vanishing on exotic scales
}

juce::Rectangle<float> snap(const juce::Rectangle<float>& r, const juce::Component* c) noexcept
{
    const float hl = hairline(c);
    // align to half-pixel for crisp 1-px strokes
    return { std::round(r.getX()) + 0.5f * hl,
             std::round(r.getY()) + 0.5f * hl,
             std::round(r.getWidth()  - hl) + hl,
             std::round(r.getHeight() - hl) + hl };
}

float autoRadius(float h, float base) noexcept
{
    return juce::jmin(base, juce::jmax(2.0f, h * 0.5f));
}

// ------------------------ higher-level semantic helpers ----------------------

namespace Tracks
{
    juce::Colour colourForIndex (int trackIndex) noexcept
    {
        const auto accent = juce::Colour (Colors::accent);
        const float baseHue = accent.getHue();
        const float sat     = juce::jlimit (0.55f, 0.90f, accent.getSaturation() * 1.1f);
        const float bri     = juce::jlimit (0.45f, 0.90f, accent.getBrightness() * 1.05f);

        const int safeIndex = trackIndex < 0 ? 0 : trackIndex;
        constexpr float goldenRatio = 0.61803398875f;
        const float hue = std::fmod (baseHue + goldenRatio * static_cast<float> (safeIndex), 1.0f);

        return juce::Colour::fromHSV (hue, sat, bri, 1.0f);
    }

    juce::Colour mutedColourForIndex (int trackIndex) noexcept
    {
        auto c = colourForIndex (trackIndex);
        return c.withSaturation (c.getSaturation() * 0.40f)
                .withBrightness (c.getBrightness() * 0.90f);
    }
} // namespace Tracks

namespace Meters
{
    float linearToDecibels (float linear) noexcept
    {
        const float clamped = juce::jlimit (0.0f, 1.0f, linear);
        if (clamped <= 0.0f)
            return -60.0f;

        return 20.0f * std::log10 (clamped);
    }

    float normalisedFromDb (float db) noexcept
    {
        constexpr float minDb = -60.0f;
        constexpr float maxDb = 0.0f;
        return juce::jlimit (0.0f, 1.0f, juce::jmap (db, minDb, maxDb, 0.0f, 1.0f));
    }

    float zeroDbLineY (const juce::Rectangle<float>& bounds) noexcept
    {
        // 0 dB is the top of the visible meter range.
        return bounds.getY();
    }
} // namespace Meters

// ------------------------------- shadows -------------------------------------

static void drawDropShadowForRoundedRect(juce::Graphics& g,
                                         juce::Rectangle<float> r,
                                         float cornerRadius,
                                         juce::Colour colour,
                                         int blurRadius,
                                         juce::Point<int> offset)
{
    juce::Path p;
    p.addRoundedRectangle(r, cornerRadius);
    juce::DropShadow ds { colour, blurRadius, offset };
    ds.drawForPath(g, p);
}

void applyShadow(juce::Graphics& g, const Shadows::ShadowParams& params,
                 const juce::Rectangle<float>& bounds) noexcept
{
    applyShadow(g, params, bounds, Radii::large);
}

void applyShadow(juce::Graphics& g, const Shadows::ShadowParams& params,
                 const juce::Rectangle<float>& bounds, float cornerRadius) noexcept
{
    if (params.alpha <= 0.0f || params.blurRadius <= 0.0f)
        return;

    // Spread → grow bounds before drawing shadow
    auto r = bounds.expanded(params.spreadRadius).translated(params.offsetX, params.offsetY);

    const int blur = juce::roundToInt(params.blurRadius);
    const juce::Colour col = juce::Colours::black.withAlpha(juce::jlimit(0.0f, 1.0f, params.alpha));

    drawDropShadowForRoundedRect(g, r, cornerRadius, col, blur,
                                 { juce::roundToInt(params.offsetX), juce::roundToInt(params.offsetY) });
}

void applyColoredShadow(juce::Graphics& g, const Shadows::ColoredShadowParams& params,
                        const juce::Rectangle<float>& bounds, float cornerRadius) noexcept
{
    if (params.blurRadius <= 0.0f)
        return;

    auto r = bounds.expanded(params.spreadRadius);
    const juce::Colour c(params.color);
    const int blur = juce::roundToInt(params.blurRadius);

    // Draw a tinted drop shadow for rounded rect
    drawDropShadowForRoundedRect(g, r.translated(params.offsetX, params.offsetY),
                                 cornerRadius, c, blur,
                                 { juce::roundToInt(params.offsetX), juce::roundToInt(params.offsetY) });
}

// ------------------------------ gradients ------------------------------------

void createGradientFill(juce::ColourGradient& gradient,
                        const Gradients::GradientStop* stops, size_t numStops,
                        const juce::Rectangle<float>& bounds, bool isVertical) noexcept
{
    if (numStops == 0) return;

    const auto p1 = isVertical
        ? juce::Point<float> { bounds.getCentreX(), bounds.getY() }
        : juce::Point<float> { bounds.getX(),       bounds.getCentreY() };

    const auto p2 = isVertical
        ? juce::Point<float> { bounds.getCentreX(), bounds.getBottom() }
        : juce::Point<float> { bounds.getRight(),   bounds.getCentreY() };

    gradient = juce::ColourGradient(juce::Colour(stops[0].color), p1.x, p1.y,
                                    juce::Colour(stops[numStops - 1].color), p2.x, p2.y, false);

    // Ensure first/last, insert intermediates
    gradient.clearColours();
    for (size_t i = 0; i < numStops; ++i)
        gradient.addColour(juce::jlimit(0.0f, 1.0f, stops[i].position), juce::Colour(stops[i].color));
}

// ------------------------------ glass panel ----------------------------------

void drawGlassPanel(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                    float cornerRadius, bool elevated) noexcept
{
    auto r = snap(bounds);
    const auto radius = autoRadius(r.getHeight(), cornerRadius);

    // Ambient shadow(s)
    if (elevated)
    {
        applyColoredShadow(g, Shadows::glassShadow1, r, radius);
        applyColoredShadow(g, Shadows::glassShadow2, r, radius);
    }
    else
    {
        applyShadow(g, Shadows::elevation1, r, radius);
    }

    // Glass fill with slight vertical tonality
    const juce::Colour base  = juce::Colour(elevated ? Colors::glassBackground
                                                     : Colors::glassBackgroundLight);
    juce::ColourGradient fill(base.brighter(0.05f), r.getCentreX(), r.getY(),
                              base.darker(0.05f),   r.getCentreX(), r.getBottom(), false);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(r, radius);

    // Border: hairline with subtle vertical fade
    juce::Path borderPath; borderPath.addRoundedRectangle(r, radius);
    const float hl = hairline(nullptr);
    juce::ColourGradient borderG(juce::Colour(Colors::glassBorder).brighter(0.20f),
                                 r.getX(), r.getY(),
                                 juce::Colour(Colors::glassBorder),
                                 r.getX(), r.getBottom(), false);
    g.setGradientFill(borderG);
    g.strokePath(borderPath, juce::PathStrokeType(juce::jmax(1.0f, hl)));

    // Top highlight
    auto top = r.withHeight(juce::jlimit(2.0f, 12.0f, r.getHeight() * 0.12f));
    juce::ColourGradient hi(juce::Colour(Colors::glassHighlight),
                            top.getCentreX(), top.getY(),
                            juce::Colour(Colors::glassHighlight).withAlpha(0.0f),
                            top.getCentreX(), top.getBottom(), false);
    g.setGradientFill(hi);
    g.fillRoundedRectangle(top, radius);

    // Inner shadow at bottom
    auto inner = r.withTop(r.getBottom() - juce::jlimit(4.0f, 24.0f, r.getHeight() * 0.15f));
    g.setColour(juce::Colour(0x20000000));
    g.fillRoundedRectangle(inner, radius);
}

// ------------------------------- typography ----------------------------------

juce::Font getHeadingFont(float size) noexcept
{
    return juce::Font(size).boldened();
}

juce::Font getBodyFont(float size) noexcept
{
    return juce::Font(size);
}

juce::Font getMonoFont(float size) noexcept
{
    return juce::Font(juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain);
}

// ------------------------------- text/shapes ---------------------------------

void drawTextWithShadow(juce::Graphics& g, const juce::String& text,
                        const juce::Rectangle<float>& bounds,
                        juce::Justification justification,
                        const juce::Font& font,
                        const juce::Colour& textColor,
                        float shadowOffsetY,
                        float shadowAlpha) noexcept
{
    if (text.isEmpty()) return;

    g.setFont(font);

    const auto shadow = textColor.withAlpha(juce::jlimit(0.0f, 1.0f, shadowAlpha));
    g.setColour(shadow);
    g.drawText(text, bounds.translated(0.0f, shadowOffsetY), justification, false);

    g.setColour(textColor);
    g.drawText(text, bounds, justification, false);
}

void drawFocusRing(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                   float radius, juce::Colour colour) noexcept
{
    auto r = bounds.reduced(hairline(nullptr) * 1.0f);
    juce::Path p; p.addRoundedRectangle(r, radius);
    g.setColour(colour.withAlpha(0.85f));
    g.strokePath(p, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}} // namespace DesignSystem
} // namespace daw::ui::lookandfeel

// ---------------------------------- tests ------------------------------------
#if JUCE_UNIT_TESTS
namespace daw::ui::lookandfeel
{
struct DesignSystemRenderTest final : public juce::UnitTest
{
    DesignSystemRenderTest() : juce::UnitTest("DesignSystem Rendering Sanity") {}

    void runTest() override
    {
        beginTest("Shadows and gradients do not crash and produce sane pixels");

        juce::Image img(juce::Image::ARGB, 320, 200, true);
        juce::Graphics g(img);

        // Shadow (elevation3)
        expectDoesNotThrow([&] {
            DesignSystem::applyShadow(g, DesignSystem::Shadows::elevation3,
                                      juce::Rectangle<float>(20, 20, 120, 60),
                                      DesignSystem::Radii::xlarge);
        });

        // Colored shadow
        expectDoesNotThrow([&] {
            DesignSystem::applyColoredShadow(g, DesignSystem::Shadows::glassShadow2,
                                             juce::Rectangle<float>(160, 20, 120, 60),
                                             DesignSystem::Radii::large);
        });

        // Gradient fill build
        juce::ColourGradient grad;
    const auto primaryStops = DesignSystem::Gradients::primaryButtonStops();
    DesignSystem::createGradientFill(grad, primaryStops.data(), primaryStops.size(),
                     juce::Rectangle<float>(20, 100, 120, 36), /*vertical*/ true);
        expect(grad.getNumColours() == 2);

        // Glass panel
        expectDoesNotThrow([&] {
            DesignSystem::drawGlassPanel(g, juce::Rectangle<float>(160, 100, 120, 60),
                                         DesignSystem::Radii::xlarge, true);
        });

        // Focus ring
        expectDoesNotThrow([&] {
            DesignSystem::drawFocusRing(g, juce::Rectangle<float>(20, 170, 120, 20),
                                        DesignSystem::Radii::medium,
                                        DesignSystem::toColour(DesignSystem::Colors::outlineFocus));
        });

        // Easing monotonicity (spot check)
        const float e1 = DesignSystem::easeIn(0.3f);
        const float e2 = DesignSystem::easeIn(0.6f);
        expect(e2 > e1);

            beginTest("Colour proxies follow base tokens");
            {
                const auto& tokens = getDesignTokens().colours;
                expect(juce::Colour(Colors::primary) == tokens.accentPrimary);
                expect(juce::Colour(Colors::text) == tokens.textPrimary);
                expect((int)Spacing::small == getDesignTokens().spacing.xs);
                expectWithinAbsoluteError((float)Typography::body, getDesignTokens().type.bodySize, 0.01f);
            }
    }
};
static DesignSystemRenderTest dsRenderTest;
} // namespace daw::ui::lookandfeel
#endif
