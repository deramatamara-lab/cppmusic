#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace daw::ui::effects
{

/**
 * GlassMorphismRenderer - Ultra-Premium Material Design
 * Implements sophisticated glass morphism effects for modern UI components.
 */
class GlassMorphismRenderer
{
public:
    struct GlassProperties
    {
        float blur = 20.0f;                    // Backdrop blur intensity
        float transparency = 0.1f;             // Glass transparency (0.0 = opaque, 1.0 = invisible)
        float frosting = 0.3f;                  // Frosting effect intensity
        juce::Colour tint = juce::Colours::white.withAlpha(0.1f); // Glass tint color
        float borderOpacity = 0.2f;            // Border visibility
        float highlightIntensity = 0.15f;      // Highlight reflection intensity
        bool enableNoise = true;               // Add subtle noise texture
        float noiseIntensity = 0.02f;          // Noise texture intensity
    };

    enum class GlassStyle
    {
        Standard,        // Classic glass morphism
        Frosted,         // Heavy frosted glass effect
        Crystal,         // Crystal-clear with highlights
        Tinted,          // Colored glass with tint
        Holographic      // Holographic glass with shifting colors
    };

    static void renderGlassPanel(juce::Graphics& g, juce::Rectangle<float> bounds,
                                const GlassProperties& props,
                                GlassStyle style = GlassStyle::Standard);

    static void renderGlassPanel(juce::Graphics& g, juce::Rectangle<float> bounds,
                                GlassStyle style = GlassStyle::Standard);

    static void renderGlassButton(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 bool isPressed, bool isHovered,
                                 const GlassProperties& props);

    static void renderGlassButton(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 bool isPressed, bool isHovered);

    static void renderGlassSlider(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 float value, const GlassProperties& props);

    static void renderGlassSlider(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 float value);

    static void renderGlassKnob(juce::Graphics& g, juce::Point<float> center, float radius,
                               float value, const GlassProperties& props);

    static void renderGlassKnob(juce::Graphics& g, juce::Point<float> center, float radius,
                               float value);

private:
    static void applyBackdropBlur(juce::Graphics& g, juce::Rectangle<float> bounds, float intensity);
    static void renderGlassGradient(juce::Graphics& g, juce::Rectangle<float> bounds,
                                   const GlassProperties& props, GlassStyle style);
    static void renderGlassBorder(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 const GlassProperties& props);
    static void renderGlassHighlights(juce::Graphics& g, juce::Rectangle<float> bounds,
                                     const GlassProperties& props);
    static void renderNoiseTexture(juce::Graphics& g, juce::Rectangle<float> bounds,
                                  float intensity);
    static void renderHolographicEffect(juce::Graphics& g, juce::Rectangle<float> bounds);
};

} // namespace daw::ui::effects

