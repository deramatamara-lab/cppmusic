#include "EnhancedMainLookAndFeel.h"
#include "DesignSystem.h"
#include "../effects/GlassMorphismRenderer.h"
#include "../effects/UltraSleekEffects.h"
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

namespace daw::ui::lookandfeel
{

using namespace DesignSystem;
using namespace daw::ui::effects;

EnhancedMainLookAndFeel::EnhancedMainLookAndFeel(Theme theme)
    : MainLookAndFeel(theme)
{
}

void EnhancedMainLookAndFeel::drawEnhancedPanel(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                bool isHighlighted, bool useGlassmorphism)
{
    if (useGlassmorphism)
    {
        GlassMorphismRenderer::renderGlassPanel(g, bounds,
            isHighlighted ? GlassMorphismRenderer::GlassStyle::Crystal :
                           GlassMorphismRenderer::GlassStyle::Standard);
    }
    else
    {
        drawPanelBackground(g, bounds);
    }

    if (isHighlighted)
    {
        drawGlowEffect(g, bounds, toColour(Colors::primary), 0.3f);
    }
}

void EnhancedMainLookAndFeel::drawProfessionalMeter(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                    float level, float peakLevel, bool isVertical)
{
    drawGradientMeter(g, bounds, level, isVertical);

    // Draw peak hold
    if (peakLevel > 0.01f)
    {
        auto peakPos = isVertical ?
            bounds.getY() + bounds.getHeight() * (1.0f - peakLevel) :
            bounds.getX() + bounds.getWidth() * peakLevel;

        g.setColour(toColour(Colors::meterDanger));
        if (isVertical)
            g.drawHorizontalLine(static_cast<int>(peakPos), bounds.getX(), bounds.getRight());
        else
            g.drawVerticalLine(static_cast<int>(peakPos), bounds.getY(), bounds.getBottom());
    }
}

void EnhancedMainLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour& backgroundColour,
                                                  bool shouldDrawButtonAsHighlighted,
                                                  bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    juce::Colour baseColour = backgroundColour;
    if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker(0.3f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.1f);

    // Draw glassmorphism background
    GlassMorphismRenderer::renderGlassPanel(g, bounds,
        shouldDrawButtonAsDown ? GlassMorphismRenderer::GlassStyle::Crystal :
                               GlassMorphismRenderer::GlassStyle::Standard);

    // Draw glow effect for highlighted state
    if (shouldDrawButtonAsHighlighted)
    {
        drawGlowEffect(g, bounds, baseColour.withAlpha(0.5f), 0.3f);
    }
}

void EnhancedMainLookAndFeel::drawGlowEffect(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                            juce::Colour glowColor, float intensity)
{
    juce::Path glowPath;
    glowPath.addRoundedRectangle(bounds, Radii::large);

    juce::DropShadow glow(glowColor.withAlpha(intensity), 8, juce::Point<int>(0, 0));
    glow.drawForPath(g, glowPath);
}

void EnhancedMainLookAndFeel::drawGradientMeter(juce::Graphics& g, juce::Rectangle<float> bounds,
                                               float level, bool isVertical)
{
    level = juce::jlimit(0.0f, 1.0f, level);

    // Determine color based on level
    juce::Colour meterColor;
    if (level < 0.7f)
        meterColor = toColour(Colors::meterNormal);
    else if (level < 0.9f)
        meterColor = toColour(Colors::meterWarning);
    else
        meterColor = toColour(Colors::meterDanger);

    // Draw meter fill
    auto fillBounds = isVertical ?
        bounds.removeFromBottom(bounds.getHeight() * level) :
        bounds.removeFromLeft(bounds.getWidth() * level);

    juce::ColourGradient gradient(
        meterColor.brighter(0.2f),
        isVertical ? fillBounds.getCentreX() : fillBounds.getX(),
        isVertical ? fillBounds.getY() : fillBounds.getCentreY(),
        meterColor.darker(0.2f),
        isVertical ? fillBounds.getCentreX() : fillBounds.getRight(),
        isVertical ? fillBounds.getBottom() : fillBounds.getCentreY(),
        !isVertical
    );

    g.setGradientFill(gradient);
    g.fillRoundedRectangle(fillBounds, Radii::small);
}

void EnhancedMainLookAndFeel::drawProfessionalWaveform(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                     const juce::AudioBuffer<float>& audioData,
                                                     juce::Colour waveformColor)
{
    if (audioData.getNumSamples() == 0)
        return;

    const auto numChannels = audioData.getNumChannels();
    const auto numSamples = audioData.getNumSamples();
    const auto channelHeight = bounds.getHeight() / static_cast<float>(numChannels);

    g.setColour(waveformColor);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto channelBounds = bounds.removeFromTop(channelHeight);
        const auto* channelData = audioData.getReadPointer(channel);

        juce::Path waveformPath;
        const auto centerY = channelBounds.getCentreY();

        waveformPath.startNewSubPath(channelBounds.getX(), centerY);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto x = channelBounds.getX() + (static_cast<float>(sample) / static_cast<float>(numSamples)) * channelBounds.getWidth();
            const auto y = centerY - (channelData[sample] * channelBounds.getHeight() * 0.5f);
            waveformPath.lineTo(x, y);
        }

        g.strokePath(waveformPath, juce::PathStrokeType(1.5f));
    }
}

} // namespace daw::ui::lookandfeel

