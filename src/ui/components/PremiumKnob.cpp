#include "PremiumKnob.h"
#include "../lookandfeel/DesignSystem.h"
#include "../core/PhysicsAnimation.h"
#include <cmath>

namespace daw::ui::components
{

using namespace daw::ui::lookandfeel::DesignSystem;

PremiumKnob::PremiumKnob()
{
    setSliderStyle(juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setLookAndFeel(nullptr); // Use custom painting

    // Enable smooth animations at 60fps
    startTimerHz(60);
}

PremiumKnob::~PremiumKnob()
{
    stopTimer();
}

void PremiumKnob::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    switch (knobStyle)
    {
        case KnobStyle::Classic:
            paintClassicKnob(g, bounds);
            break;
        case KnobStyle::Modern:
            paintModernKnob(g, bounds);
            break;
        case KnobStyle::Spectrum:
            paintSpectrumKnob(g, bounds);
            break;
        case KnobStyle::Vintage:
            paintVintageKnob(g, bounds);
            break;
        case KnobStyle::Futuristic:
            paintFuturisticKnob(g, bounds);
            break;
    }
}

void PremiumKnob::paintModernKnob(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    auto centre = bounds.getCentre();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.4f;
    auto trackRadius = radius * 1.2f;

    // Calculate rotation angle
    auto rotaryStartAngle = juce::MathConstants<float>::pi * 1.2f;
    auto rotaryEndAngle = juce::MathConstants<float>::pi * 2.8f;
    auto angle = rotaryStartAngle + (getValue() - getMinimum()) / (getMaximum() - getMinimum()) * (rotaryEndAngle - rotaryStartAngle);

    // Draw track background
    juce::Path track;
    track.addArc(centre.x - trackRadius, centre.y - trackRadius,
                trackRadius * 2.0f, trackRadius * 2.0f,
                rotaryStartAngle, rotaryEndAngle, true);

    g.setColour(toColour(Colors::surface));
    g.strokePath(track, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved));

    // Draw value track with gradient
    if (getValue() > getMinimum())
    {
        juce::Path valueTrack;
        valueTrack.addArc(centre.x - trackRadius, centre.y - trackRadius,
                         trackRadius * 2.0f, trackRadius * 2.0f,
                         rotaryStartAngle, angle, true);

        // Audio-reactive color
        auto baseColor = toColour(Colors::primary);
        if (audioReactive && audioLevel > 0.1f)
        {
            float intensity = juce::jlimit(0.0f, 1.0f, audioLevel * 2.0f);
            baseColor = baseColor.brighter(intensity * 0.3f);
        }

        g.setColour(baseColor);
        g.strokePath(valueTrack, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Draw knob body with premium gradient
    juce::ColourGradient knobGradient(
        toColour(Colors::surfaceElevated),
        centre.x, centre.y - radius * 0.5f,
        toColour(Colors::surface),
        centre.x, centre.y + radius * 0.5f,
        false
    );

    g.setGradientFill(knobGradient);
    g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    // Draw subtle border
    g.setColour(toColour(Colors::primary).withAlpha(0.3f + hoverProgress.value * 0.4f));
    g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);

    // Draw pointer
    auto pointerLength = radius * 0.7f;
    juce::Point<float> pointer(
        centre.x + std::cos(angle) * pointerLength,
        centre.y + std::sin(angle) * pointerLength);

    g.setColour(toColour(Colors::primary));
    g.drawLine(centre.x, centre.y, pointer.x, pointer.y, 2.0f);
}

void PremiumKnob::paintClassicKnob(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    paintModernKnob(g, bounds); // Use modern as base for now
}

void PremiumKnob::paintSpectrumKnob(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    paintModernKnob(g, bounds); // Use modern as base for now
}

void PremiumKnob::paintVintageKnob(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    paintModernKnob(g, bounds); // Use modern as base for now
}

void PremiumKnob::paintFuturisticKnob(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    paintModernKnob(g, bounds); // Use modern as base for now
}

void PremiumKnob::mouseEnter(const juce::MouseEvent& event)
{
    hoverProgress.setTarget(1.0f);
    glowIntensity.setTarget(0.5f);
    Slider::mouseEnter(event);
}

void PremiumKnob::mouseExit(const juce::MouseEvent& event)
{
    hoverProgress.setTarget(0.0f);
    glowIntensity.setTarget(0.0f);
    Slider::mouseExit(event);
}

void PremiumKnob::mouseDown(const juce::MouseEvent& event)
{
    Slider::mouseDown(event);
}

void PremiumKnob::mouseUp(const juce::MouseEvent& event)
{
    Slider::mouseUp(event);
}

void PremiumKnob::mouseDrag(const juce::MouseEvent& event)
{
    Slider::mouseDrag(event);
}

void PremiumKnob::timerCallback()
{
    bool stillAnimating = hoverProgress.update(0.016f) || glowIntensity.update(0.016f);

    if (stillAnimating)
    {
        repaint();
    }
    else
    {
        stopTimer();
    }
}

} // namespace daw::ui::components

