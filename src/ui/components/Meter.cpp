#include "Meter.h"
#include <algorithm>
#include <cmath>

namespace daw::ui::components
{

Meter::Meter(Orientation orientation)
    : orientation(orientation)
{
    startTimer(30); // 30fps update
}

void Meter::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Background
    g.fillAll(toColour(Colors::meterBackground));
    
    if (orientation == Orientation::Vertical)
    {
        drawVerticalMeter(g);
    }
    else
    {
        drawHorizontalMeter(g);
    }
}

void Meter::timerCallback()
{
    repaint();
}

void Meter::setLevel(float level)
{
    currentLevel.store(juce::jmax(0.0f, level), std::memory_order_release);
    
    // Update peak hold
    if (peakHoldEnabled)
    {
        const auto current = currentLevel.load(std::memory_order_acquire);
        float peak = peakLevel.load(std::memory_order_acquire);
        if (current > peak)
        {
            peakLevel.store(current, std::memory_order_release);
        }
        else
        {
            // Decay peak
            peak *= 0.99f;
            peakLevel.store(peak, std::memory_order_release);
        }
    }
}

void Meter::setPeakHold(bool enabled)
{
    peakHoldEnabled = enabled;
    if (!enabled)
    {
        resetPeakHold();
    }
}

void Meter::resetPeakHold()
{
    peakLevel.store(0.0f, std::memory_order_release);
}

void Meter::setOrientation(Orientation newOrientation)
{
    orientation = newOrientation;
    repaint();
}

void Meter::drawVerticalMeter(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    const auto bounds = getLocalBounds().toFloat();
    const auto level = currentLevel.load(std::memory_order_acquire);
    const auto peak = peakLevel.load(std::memory_order_acquire);
    
    // Calculate meter height
    const auto meterHeight = bounds.getHeight() * level;
    const auto meterY = bounds.getBottom() - meterHeight;
    const auto meterBounds = juce::Rectangle<float>(bounds.getX(), meterY, bounds.getWidth(), meterHeight);
    
    // Draw meter with gradient
    if (meterHeight > 0.0f)
    {
        juce::ColourGradient gradient(
            getMeterColour(level),
            meterBounds.getTopLeft(),
            getMeterColour(0.0f),
            meterBounds.getBottomLeft(),
            false
        );
        g.setGradientFill(gradient);
        g.fillRect(meterBounds);
    }
    
    // Draw peak hold indicator
    if (peakHoldEnabled && peak > 0.0f)
    {
        const auto peakY = bounds.getBottom() - (bounds.getHeight() * peak);
        g.setColour(toColour(Colors::text));
        g.drawHorizontalLine(static_cast<int>(peakY), bounds.getX(), bounds.getRight());
    }
}

void Meter::drawHorizontalMeter(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    const auto bounds = getLocalBounds().toFloat();
    const auto level = currentLevel.load(std::memory_order_acquire);
    const auto peak = peakLevel.load(std::memory_order_acquire);
    
    // Calculate meter width
    const auto meterWidth = bounds.getWidth() * level;
    const auto meterBounds = juce::Rectangle<float>(bounds.getX(), bounds.getY(), meterWidth, bounds.getHeight());
    
    // Draw meter with gradient
    if (meterWidth > 0.0f)
    {
        juce::ColourGradient gradient(
            getMeterColour(level),
            meterBounds.getTopLeft(),
            getMeterColour(0.0f),
            meterBounds.getTopRight(),
            false
        );
        g.setGradientFill(gradient);
        g.fillRect(meterBounds);
    }
    
    // Draw peak hold indicator
    if (peakHoldEnabled && peak > 0.0f)
    {
        const auto peakX = bounds.getX() + (bounds.getWidth() * peak);
        g.setColour(toColour(Colors::text));
        g.drawVerticalLine(static_cast<int>(peakX), bounds.getY(), bounds.getBottom());
    }
}

juce::Colour Meter::getMeterColour(float level) const
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    if (level < 0.7f)
    {
        return toColour(Colors::meterNormal);
    }
    else if (level < 0.9f)
    {
        return toColour(Colors::meterWarning);
    }
    else
    {
        return toColour(Colors::meterDanger);
    }
}

} // namespace daw::ui::components

