#include "Panel.h"

namespace daw::ui::components
{

Panel::Panel()
{
    setInterceptsMouseClicks(true, true);
}

void Panel::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    const auto bounds = getLocalBounds().toFloat();
    const auto cornerRadius = Radii::large;
    
    // Draw panel background with gradient
    juce::ColourGradient gradient;
    if (isElevated)
    {
        gradient = juce::ColourGradient(
            toColour(Colors::surfaceElevated),
            bounds.getTopLeft(),
            toColour(Colors::surface),
            bounds.getBottomLeft(),
            false
        );
    }
    else
    {
        gradient = juce::ColourGradient(
            toColour(Colors::surface),
            bounds.getTopLeft(),
            toColour(Colors::background),
            bounds.getBottomLeft(),
            false
        );
    }
    
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerRadius);
    
    // Draw border
    g.setColour(toColour(Colors::outline));
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
    
    // Draw header if enabled
    if (showHeader && title.isNotEmpty())
    {
        const auto headerBounds = getHeaderBounds().toFloat();
        
        // Header background
        g.setColour(toColour(Colors::surfaceElevated));
        g.fillRoundedRectangle(headerBounds, cornerRadius);
        
        // Header text
        g.setColour(toColour(Colors::text));
        g.setFont(Typography::body);
        g.drawText(title, headerBounds, juce::Justification::centredLeft);
    }
}

void Panel::setTitle(const juce::String& newTitle)
{
    title = newTitle;
    repaint();
}

void Panel::setShowHeader(bool show)
{
    showHeader = show;
    repaint();
}

void Panel::setElevated(bool elevated)
{
    isElevated = elevated;
    repaint();
}

juce::Rectangle<int> Panel::getHeaderBounds() const
{
    auto bounds = getLocalBounds();
    const auto headerHeight = static_cast<int>(daw::ui::lookandfeel::DesignSystem::Typography::body + 
                                                daw::ui::lookandfeel::DesignSystem::Spacing::small * 2);
    return bounds.removeFromTop(headerHeight);
}

juce::Rectangle<int> Panel::getContentBounds() const
{
    auto bounds = getLocalBounds();
    if (showHeader)
    {
        const auto headerHeight = static_cast<int>(daw::ui::lookandfeel::DesignSystem::Typography::body + 
                                                    daw::ui::lookandfeel::DesignSystem::Spacing::small * 2);
        bounds.removeFromTop(headerHeight);
    }
    return bounds;
}

} // namespace daw::ui::components

