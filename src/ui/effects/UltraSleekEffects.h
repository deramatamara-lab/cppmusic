#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"
#include <cmath>

namespace daw::ui::effects
{

using namespace daw::ui::lookandfeel::DesignSystem;

/**
 * UltraSleekEffects - Advanced visual effects system
 * Advanced visual effects that go beyond basic shadows and glows
 */
class UltraSleekEffects
{
public:
    /**
     * Advanced multi-layer shadow with depth and sophistication
     */
    static void renderMultiLayerShadow(juce::Graphics& g, juce::Rectangle<float> bounds,
                                     float cornerRadius, juce::Colour shadowColor = juce::Colour(0x40000000),
                                     int layers = 3, float baseOpacity = 0.12f)
    {
        for (int i = layers; i > 0; --i)
        {
            float offset = i * 1.2f;
            float opacity = baseOpacity * (1.0f - (i / float(layers + 1)));
            float spread = i * 0.8f;
            float blur = i * 2.0f;

            // Create shadow with varying properties per layer
            juce::DropShadow layerShadow(
                shadowColor.withAlpha(opacity),
                static_cast<int>(blur),
                juce::Point<int>(static_cast<int>(offset * 0.7f), static_cast<int>(offset))
            );

            juce::Path shadowPath;
            shadowPath.addRoundedRectangle(bounds.expanded(spread), cornerRadius + spread);
            layerShadow.drawForPath(g, shadowPath);
        }
    }

    /**
     * Advanced glow with chromatic aberration effect for premium feel
     */
    static void renderChromaticGlow(juce::Graphics& g, juce::Rectangle<float> bounds,
                                  float cornerRadius, juce::Colour baseColor, float intensity)
    {
        if (intensity < 0.01f) return;

        // Save graphics state
        juce::Graphics::ScopedSaveState saveState(g);

        // Red channel (slightly offset left)
        juce::Path redPath;
        redPath.addRoundedRectangle(bounds.expanded(3.0f).translated(-1.0f, 0), cornerRadius + 3.0f);
        juce::DropShadow redGlow(juce::Colour(0xffff4444).withAlpha(intensity * 0.25f), 4, juce::Point<int>(0, 0));
        redGlow.drawForPath(g, redPath);

        // Green channel (center)
        juce::Path greenPath;
        greenPath.addRoundedRectangle(bounds.expanded(2.5f), cornerRadius + 2.5f);
        juce::DropShadow greenGlow(juce::Colour(0xff44ff44).withAlpha(intensity * 0.35f), 4, juce::Point<int>(0, 0));
        greenGlow.drawForPath(g, greenPath);

        // Blue channel (slightly offset right)
        juce::Path bluePath;
        bluePath.addRoundedRectangle(bounds.expanded(3.0f).translated(1.0f, 0), cornerRadius + 3.0f);
        juce::DropShadow blueGlow(juce::Colour(0xff4444ff).withAlpha(intensity * 0.25f), 4, juce::Point<int>(0, 0));
        blueGlow.drawForPath(g, bluePath);

        // Core glow with base color
        juce::Path corePath;
        corePath.addRoundedRectangle(bounds.expanded(2.0f), cornerRadius + 2.0f);
        juce::DropShadow coreGlow(baseColor.withAlpha(intensity * 0.6f), 4, juce::Point<int>(0, 0));
        coreGlow.drawForPath(g, corePath);
    }

    /**
     * Render subtle noise texture for premium material feel
     */
    static void renderNoiseTexture(juce::Graphics& g, juce::Rectangle<float> bounds, float intensity = 0.02f)
    {
        if (intensity < 0.001f) return;

        juce::Random random(42); // Consistent seed for repeatable noise
        int width = static_cast<int>(bounds.getWidth());
        int height = static_cast<int>(bounds.getHeight());

        // Create noise pattern
        juce::Image noiseImage(juce::Image::ARGB, width, height, true);
        juce::Graphics noiseGraphics(noiseImage);

        // Generate noise pixels with controlled density
        int numPixels = static_cast<int>(width * height * intensity * 0.1f);

        for (int i = 0; i < numPixels; ++i)
        {
            int x = random.nextInt(width);
            int y = random.nextInt(height);

            float alpha = random.nextFloat() * intensity;
            juce::Colour noiseColor = random.nextBool() ?
                juce::Colours::white.withAlpha(alpha) :
                juce::Colours::black.withAlpha(alpha * 0.7f);

            noiseGraphics.setColour(noiseColor);
            noiseGraphics.fillRect(x, y, 1, 1);
        }

        // Apply noise with reduced opacity
        g.setOpacity(0.4f);
        g.drawImage(noiseImage, bounds);
        g.setOpacity(1.0f);
    }

    /**
     * Subtle gradient overlay for depth and dimensionality
     */
    static void renderGradientOverlay(juce::Graphics& g, juce::Rectangle<float> bounds,
                                    juce::Colour topColor, juce::Colour bottomColor, float cornerRadius = 8.0f)
    {
        juce::ColourGradient overlay(
            topColor, bounds.getCentreX(), bounds.getY(),
            bottomColor, bounds.getCentreX(), bounds.getBottom(),
            false
        );

        g.setGradientFill(overlay);
        g.fillRoundedRectangle(bounds, cornerRadius);
    }

    /**
     * Render ripple effect for button interactions
     */
    static void renderRippleEffect(juce::Graphics& g, juce::Point<float> center,
                                 float progress, float alpha, juce::Colour rippleColor)
    {
        if (progress < 0.01f || alpha < 0.01f) return;

        float radius = progress * 100.0f; // Max ripple radius
        float currentAlpha = alpha * (1.0f - progress); // Fade out as it expands

        // Multiple ripple rings for sophisticated effect
        for (int ring = 0; ring < 3; ++ring)
        {
            float ringRadius = radius - (ring * 8.0f);
            if (ringRadius > 0)
            {
                float ringAlpha = currentAlpha * (1.0f - ring * 0.3f);
                g.setColour(rippleColor.withAlpha(ringAlpha));

                juce::Path ripplePath;
                ripplePath.addEllipse(center.x - ringRadius, center.y - ringRadius,
                                     ringRadius * 2.0f, ringRadius * 2.0f);
                g.strokePath(ripplePath, juce::PathStrokeType(1.5f - ring * 0.3f));
            }
        }
    }

    /**
     * Glass morphism background effect
     */
    static void renderGlassMorphismBackground(juce::Graphics& g, juce::Rectangle<float> bounds,
                                            juce::Colour baseColor, float opacity = 0.15f, float cornerRadius = 12.0f)
    {
        // Main glass background
        juce::ColourGradient glassGradient(
            baseColor.withAlpha(opacity * 1.2f),
            bounds.getX(), bounds.getY(),
            baseColor.withAlpha(opacity * 0.6f),
            bounds.getRight(), bounds.getBottom(),
            false
        );

        g.setGradientFill(glassGradient);
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Glass highlight at top
        auto highlightBounds = bounds.removeFromTop(bounds.getHeight() * 0.3f);
        juce::ColourGradient highlight(
            juce::Colours::white.withAlpha(opacity * 0.4f),
            highlightBounds.getCentreX(), highlightBounds.getY(),
            juce::Colours::transparentWhite,
            highlightBounds.getCentreX(), highlightBounds.getBottom(),
            false
        );

        g.setGradientFill(highlight);
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Subtle border
        g.setColour(juce::Colours::white.withAlpha(opacity * 0.5f));
        g.drawRoundedRectangle(bounds, cornerRadius, 0.5f);
    }

    /**
     * Advanced inner shadow for depth
     */
    static void renderInnerShadow(juce::Graphics& g, juce::Rectangle<float> bounds,
                                float cornerRadius, juce::Colour shadowColor = juce::Colour(0x60000000),
                                float size = 3.0f)
    {
        // Create inner shadow path
        juce::Path outerPath, innerPath;
        outerPath.addRoundedRectangle(bounds, cornerRadius);
        innerPath.addRoundedRectangle(bounds.reduced(size), cornerRadius - size);

        outerPath.setUsingNonZeroWinding(false);
        outerPath.addPath(innerPath);

        g.setColour(shadowColor);
        g.fillPath(outerPath);
    }

    /**
     * Holographic shifting effect for special elements
     */
    static void renderHolographicEffect(juce::Graphics& g, juce::Rectangle<float> bounds,
                                      float cornerRadius = 8.0f)
    {
        // Animated holographic colors based on time
        float time = static_cast<float>(juce::Time::getMillisecondCounter() % 4000) / 4000.0f;

        // Create shifting rainbow gradient
        juce::ColourGradient holoGradient;
        holoGradient.point1 = bounds.getTopLeft();
        holoGradient.point2 = bounds.getBottomRight();
        holoGradient.isRadial = false;

        // Add shifting color stops
        for (int i = 0; i < 5; ++i)
        {
            float hue = std::fmod(time + (i * 0.2f), 1.0f);
            juce::Colour holoColor = juce::Colour::fromHSV(hue, 0.6f, 0.9f, 0.15f);
            holoGradient.addColour(i / 4.0f, holoColor);
        }

        g.setGradientFill(holoGradient);
        g.fillRoundedRectangle(bounds, cornerRadius);
    }
};

} // namespace daw::ui::effects

