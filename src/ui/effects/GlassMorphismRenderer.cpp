#include "GlassMorphismRenderer.h"
#include "../lookandfeel/DesignSystem.h"
#include <cmath>

namespace daw::ui::effects
{

using namespace daw::ui::lookandfeel::DesignSystem;

void GlassMorphismRenderer::renderGlassPanel(juce::Graphics& g, juce::Rectangle<float> bounds,
                                            const GlassProperties& props, GlassStyle style)
{
    // Save graphics state
    juce::Graphics::ScopedSaveState saveState(g);

    // Apply backdrop blur simulation
    applyBackdropBlur(g, bounds, props.blur);

    // Render glass gradient based on style
    renderGlassGradient(g, bounds, props, style);

    // Add glass highlights for premium effect
    renderGlassHighlights(g, bounds, props);

    // Render sophisticated border
    renderGlassBorder(g, bounds, props);

    // Add noise texture for realism
    if (props.enableNoise)
        renderNoiseTexture(g, bounds, props.noiseIntensity);

    // Special effects for holographic style
    if (style == GlassStyle::Holographic)
        renderHolographicEffect(g, bounds);
}

void GlassMorphismRenderer::renderGlassPanel(juce::Graphics& g, juce::Rectangle<float> bounds,
                                            GlassStyle style)
{
    GlassProperties defaultProps;
    renderGlassPanel(g, bounds, defaultProps, style);
}

void GlassMorphismRenderer::renderGlassButton(juce::Graphics& g, juce::Rectangle<float> bounds,
                                             bool isPressed, bool isHovered,
                                             const GlassProperties& props)
{
    GlassProperties buttonProps = props;

    // Enhance glass properties based on interaction state
    if (isPressed)
    {
        buttonProps.transparency *= 0.7f; // More opaque when pressed
        buttonProps.highlightIntensity *= 1.5f;
        buttonProps.borderOpacity *= 1.3f;
    }
    else if (isHovered)
    {
        buttonProps.transparency *= 0.85f; // Slightly more opaque on hover
        buttonProps.highlightIntensity *= 1.2f;
        buttonProps.tint = buttonProps.tint.brighter(0.1f);
    }

    renderGlassPanel(g, bounds, buttonProps, GlassStyle::Crystal);

    // Add interactive glow
    if (isHovered || isPressed)
    {
        auto glowColor = toColour(Colors::primary).withAlpha(0.3f);
        juce::DropShadow glow(glowColor, 8, juce::Point<int>(0, 0));
        juce::Path glowPath;
        glowPath.addRoundedRectangle(bounds, Radii::large);
        glow.drawForPath(g, glowPath);
    }
}

void GlassMorphismRenderer::renderGlassButton(juce::Graphics& g, juce::Rectangle<float> bounds,
                                             bool isPressed, bool isHovered)
{
    GlassProperties defaultProps;
    renderGlassButton(g, bounds, isPressed, isHovered, defaultProps);
}

void GlassMorphismRenderer::renderGlassSlider(juce::Graphics& g, juce::Rectangle<float> bounds,
                                             float value, const GlassProperties& props)
{
    // Glass track background
    GlassProperties trackProps = props;
    trackProps.transparency = 0.15f;
    trackProps.frosting = 0.4f;

    renderGlassPanel(g, bounds, trackProps, GlassStyle::Frosted);

    // Glass fill with premium gradient
    if (value > 0.001f)
    {
        auto fillBounds = bounds.removeFromLeft(bounds.getWidth() * value);

        GlassProperties fillProps = props;
        fillProps.tint = toColour(Colors::primary).withAlpha(0.2f);
        fillProps.transparency = 0.05f;
        fillProps.highlightIntensity = 0.3f;

        renderGlassPanel(g, fillBounds, fillProps, GlassStyle::Tinted);
    }
}

void GlassMorphismRenderer::renderGlassSlider(juce::Graphics& g, juce::Rectangle<float> bounds,
                                             float value)
{
    GlassProperties defaultProps;
    renderGlassSlider(g, bounds, value, defaultProps);
}

void GlassMorphismRenderer::renderGlassKnob(juce::Graphics& g, juce::Point<float> center, float radius,
                                           float value, const GlassProperties& props)
{
    juce::ignoreUnused(value);

    auto knobBounds = juce::Rectangle<float>(radius * 2, radius * 2).withCentre(center);

    // Glass knob body
    GlassProperties knobProps = props;
    knobProps.transparency = 0.08f;
    knobProps.frosting = 0.5f;
    knobProps.highlightIntensity = 0.25f;

    // Create circular clip region
    juce::Path clipPath;
    clipPath.addEllipse(knobBounds);
    g.reduceClipRegion(clipPath);

    renderGlassPanel(g, knobBounds, knobProps, GlassStyle::Crystal);

    g.resetToDefaultState();

    // Glass border
    g.setColour(juce::Colours::white.withAlpha(props.borderOpacity * 0.8f));
    g.drawEllipse(knobBounds, 1.5f);
}

void GlassMorphismRenderer::renderGlassKnob(juce::Graphics& g, juce::Point<float> center, float radius,
                                           float value)
{
    GlassProperties defaultProps;
    renderGlassKnob(g, center, radius, value, defaultProps);
}

void GlassMorphismRenderer::applyBackdropBlur(juce::Graphics& g, juce::Rectangle<float> bounds, float intensity)
{
    // Simulate backdrop blur with multiple gradient layers
    int numLayers = static_cast<int>(intensity / 5.0f) + 1;

    for (int i = 0; i < numLayers; ++i)
    {
        float layerAlpha = (intensity / 100.0f) * (1.0f - i * 0.2f);
        juce::Colour blurColor = juce::Colours::white.withAlpha(layerAlpha * 0.05f);

        auto layerBounds = bounds.expanded(i * 2.0f);

        juce::ColourGradient blurGradient(
            blurColor, layerBounds.getCentreX(), layerBounds.getY(),
            juce::Colours::transparentWhite, layerBounds.getCentreX(), layerBounds.getBottom(),
            false
        );

        g.setGradientFill(blurGradient);
        g.fillRoundedRectangle(layerBounds, Radii::large);
    }
}

void GlassMorphismRenderer::renderGlassGradient(juce::Graphics& g, juce::Rectangle<float> bounds,
                                               const GlassProperties& props, GlassStyle style)
{
    juce::ColourGradient gradient;

    switch (style)
    {
        case GlassStyle::Standard:
        {
            gradient = juce::ColourGradient(
                juce::Colours::white.withAlpha(0.15f * (1.0f - props.transparency)),
                bounds.getX(), bounds.getY(),
                juce::Colours::white.withAlpha(0.05f * (1.0f - props.transparency)),
                bounds.getX(), bounds.getBottom(),
                false
            );
            break;
        }

        case GlassStyle::Frosted:
        {
            auto frostColor = juce::Colours::white.withAlpha(props.frosting * 0.3f);
            gradient = juce::ColourGradient(
                frostColor.brighter(0.1f),
                bounds.getCentreX(), bounds.getY(),
                frostColor.darker(0.1f),
                bounds.getCentreX(), bounds.getBottom(),
                false
            );
            break;
        }

        case GlassStyle::Crystal:
        {
            gradient = juce::ColourGradient(
                juce::Colours::white.withAlpha(0.25f * (1.0f - props.transparency)),
                bounds.getX(), bounds.getY(),
                juce::Colours::white.withAlpha(0.02f * (1.0f - props.transparency)),
                bounds.getX(), bounds.getBottom(),
                false
            );
            break;
        }

        case GlassStyle::Tinted:
        {
            auto tintedStart = props.tint.withMultipliedAlpha(1.0f - props.transparency);
            auto tintedEnd = props.tint.withMultipliedAlpha((1.0f - props.transparency) * 0.3f);

            gradient = juce::ColourGradient(
                tintedStart, bounds.getX(), bounds.getY(),
                tintedEnd, bounds.getX(), bounds.getBottom(),
                false
            );
            break;
        }

        case GlassStyle::Holographic:
        {
            // Multi-color holographic gradient
            gradient = juce::ColourGradient(
                juce::Colours::cyan.withAlpha(0.1f),
                bounds.getX(), bounds.getY(),
                juce::Colours::magenta.withAlpha(0.1f),
                bounds.getRight(), bounds.getBottom(),
                false
            );
            gradient.addColour(0.33, juce::Colours::lime.withAlpha(0.08f));
            gradient.addColour(0.66, juce::Colours::orange.withAlpha(0.08f));
            break;
        }
    }

    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, Radii::large);
}

void GlassMorphismRenderer::renderGlassBorder(juce::Graphics& g, juce::Rectangle<float> bounds,
                                             const GlassProperties& props)
{
    // Sophisticated glass border with gradient
    juce::ColourGradient borderGradient(
        juce::Colours::white.withAlpha(props.borderOpacity * 0.8f),
        bounds.getCentreX(), bounds.getY(),
        juce::Colours::white.withAlpha(props.borderOpacity * 0.2f),
        bounds.getCentreX(), bounds.getBottom(),
        false
    );

    g.setGradientFill(borderGradient);
    g.drawRoundedRectangle(bounds, Radii::large, 1.0f);

    // Inner border highlight
    juce::ColourGradient innerBorderGradient(
        juce::Colours::white.withAlpha(props.borderOpacity * 0.4f),
        bounds.getCentreX(), bounds.getY(),
        juce::Colours::transparentWhite,
        bounds.getCentreX(), bounds.getY() + bounds.getHeight() * 0.3f,
        false
    );

    g.setGradientFill(innerBorderGradient);
    g.drawRoundedRectangle(bounds.reduced(1.0f), Radii::large - 1.0f, 0.5f);
}

void GlassMorphismRenderer::renderGlassHighlights(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                 const GlassProperties& props)
{
    // Top highlight reflection
    auto highlightArea = bounds.removeFromTop(bounds.getHeight() * 0.4f);

    juce::ColourGradient highlightGradient(
        juce::Colours::white.withAlpha(props.highlightIntensity),
        highlightArea.getCentreX(), highlightArea.getY(),
        juce::Colours::transparentWhite,
        highlightArea.getCentreX(), highlightArea.getBottom(),
        false
    );

    g.setGradientFill(highlightGradient);
    g.fillRoundedRectangle(highlightArea, Radii::large);

    // Side highlight reflections
    auto leftHighlight = bounds.removeFromLeft(2.0f);
    g.setColour(juce::Colours::white.withAlpha(props.highlightIntensity * 0.6f));
    g.fillRoundedRectangle(leftHighlight, 1.0f);
}

void GlassMorphismRenderer::renderNoiseTexture(juce::Graphics& g, juce::Rectangle<float> bounds,
                                              float intensity)
{
    // Simulate subtle glass texture with noise pattern
    juce::Random random(42); // Consistent seed for repeatable noise

    int numPoints = static_cast<int>(bounds.getWidth() * bounds.getHeight() / 100.0f);

    for (int i = 0; i < numPoints; ++i)
    {
        float x = bounds.getX() + random.nextFloat() * bounds.getWidth();
        float y = bounds.getY() + random.nextFloat() * bounds.getHeight();

        float alpha = random.nextFloat() * intensity;
        juce::Colour noiseColor = random.nextBool() ?
            juce::Colours::white.withAlpha(alpha) :
            juce::Colours::black.withAlpha(alpha * 0.5f);

        g.setColour(noiseColor);
        g.fillEllipse(x, y, 1.0f, 1.0f);
    }
}

void GlassMorphismRenderer::renderHolographicEffect(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Animated holographic color shifts
    float time = static_cast<float>(juce::Time::getMillisecondCounter() % 5000) / 5000.0f;

    // Multiple iridescent layers
    for (int i = 0; i < 3; ++i)
    {
        float phaseOffset = i * 0.33f;
        float hue = std::fmod(time + phaseOffset, 1.0f);

        auto holoColor = juce::Colour::fromHSV(hue, 0.3f, 0.8f, 0.05f);

        juce::ColourGradient holoGradient(
            holoColor,
            bounds.getX() + i * bounds.getWidth() * 0.1f, bounds.getY(),
            juce::Colours::transparentBlack,
            bounds.getX() + i * bounds.getWidth() * 0.1f, bounds.getBottom(),
            false
        );

        g.setGradientFill(holoGradient);
        g.fillRoundedRectangle(bounds, Radii::large);
    }
}

} // namespace daw::ui::effects

