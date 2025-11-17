#include "Dial.h"
#include <cmath>

namespace daw::ui::components
{

Dial::Dial()
{
    setSliderStyle(juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setInterceptsMouseClicks(true, true);
}

void Dial::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    const auto bounds = getLocalBounds().toFloat();
    const auto centre = bounds.getCentre();
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.4f;
    const auto knobBounds = juce::Rectangle<float>(centre.x - radius, centre.y - radius, radius * 2, radius * 2);
    
    // Draw halo
    drawHalo(g, knobBounds);
    
    // Draw modulation ring if enabled
    if (showModulationRing)
    {
        drawModulationRing(g, knobBounds);
    }
    
    // Draw knob
    drawKnob(g, knobBounds);
}

void Dial::mouseDown(const juce::MouseEvent& e)
{
    lastMousePosition = e.position;
    juce::Slider::mouseDown(e);
}

void Dial::mouseDrag(const juce::MouseEvent& e)
{
    const auto delta = e.position.y - lastMousePosition.y;
    const auto sensitivity = e.mods.isShiftDown() ? 0.1f : 1.0f;
    const auto valueChange = -delta * sensitivity * 0.01f;
    
    setValue(getValue() + valueChange * (getMaximum() - getMinimum()));
    lastMousePosition = e.position;
    
    repaint();
}

void Dial::setModulationDepth(float depth)
{
    modulationDepth = juce::jlimit(0.0f, 1.0f, depth);
    repaint();
}

void Dial::setShowModulationRing(bool show)
{
    showModulationRing = show;
    repaint();
}

void Dial::drawHalo(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    const auto centre = bounds.getCentre();
    const auto radius = bounds.getWidth() * 0.5f;
    const auto haloRadius = radius * 1.2f;
    
    // Draw outer glow
    g.setColour(toColour(Colors::primary).withAlpha(0.2f));
    for (int i = 0; i < 3; ++i)
    {
        const auto glowRadius = haloRadius + i * 2.0f;
        g.drawEllipse(centre.x - glowRadius, centre.y - glowRadius, glowRadius * 2, glowRadius * 2, 1.0f);
    }
}

void Dial::drawKnob(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    const auto centre = bounds.getCentre();
    const auto radius = bounds.getWidth() * 0.5f;
    
    // Draw knob body with gradient
    juce::ColourGradient gradient(
        toColour(Colors::surfaceElevated),
        bounds.getTopLeft(),
        toColour(Colors::surface),
        bounds.getBottomLeft(),
        false
    );
    g.setGradientFill(gradient);
    g.fillEllipse(bounds);
    
    // Draw border
    g.setColour(toColour(Colors::outline));
    g.drawEllipse(bounds, 2.0f);
    
    // Draw value arc
    const auto normalizedValue = static_cast<float>((getValue() - getMinimum()) / (getMaximum() - getMinimum()));
    const auto startAngle = -2.5f;
    const auto endAngle = startAngle + normalizedValue * 5.0f;
    
    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius * 0.8f, radius * 0.8f, 0.0f, startAngle, endAngle, true);
    g.setColour(toColour(Colors::primary));
    g.strokePath(arc, juce::PathStrokeType(3.0f));
    
    // Draw pointer
    const auto pointerAngle = startAngle + normalizedValue * 5.0f;
    const auto pointerLength = radius * 0.6f;
    const auto pointerX = centre.x + std::cos(pointerAngle) * pointerLength;
    const auto pointerY = centre.y + std::sin(pointerAngle) * pointerLength;
    
    g.setColour(toColour(Colors::accent));
    g.fillEllipse(pointerX - 3.0f, pointerY - 3.0f, 6.0f, 6.0f);
}

void Dial::drawModulationRing(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    if (modulationDepth <= 0.0f)
        return;
    
    const auto centre = bounds.getCentre();
    const auto radius = bounds.getWidth() * 0.5f;
    const auto ringRadius = radius * 1.1f;
    
    // Draw modulation depth arc
    const auto modAngle = modulationDepth * 2.0f * juce::MathConstants<float>::pi;
    juce::Path modArc;
    modArc.addCentredArc(centre.x, centre.y, ringRadius, ringRadius, 0.0f, 0.0f, modAngle, false);
    
    g.setColour(toColour(Colors::accent).withAlpha(0.6f));
    g.strokePath(modArc, juce::PathStrokeType(2.0f));
}

float Dial::getValueFromPosition(const juce::Point<int>& pos) const
{
    juce::ignoreUnused(pos);
    // Implementation would convert mouse position to value
    return 0.0f;
}

} // namespace daw::ui::components

