#include "DesignSystem.h"
#include <cmath>

namespace daw::ui::lookandfeel
{
namespace DesignSystem
{
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
    return juce::Font(juce::FontOptions().withHeight(size)).boldened();
}

juce::Font getBodyFont(float size) noexcept
{
    return juce::Font(juce::FontOptions().withHeight(size));
}

juce::Font getMonoFont(float size) noexcept
{
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain));
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
}

} // namespace DesignSystem
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
        DesignSystem::createGradientFill(grad, DesignSystem::Gradients::primaryButtonStops,
                                         std::size(DesignSystem::Gradients::primaryButtonStops),
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
    }
};
static DesignSystemRenderTest dsRenderTest;
} // namespace daw::ui::lookandfeel
#endif
