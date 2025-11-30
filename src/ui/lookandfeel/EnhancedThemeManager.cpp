#include "EnhancedThemeManager.h"
#include "DesignSystem.h"

namespace daw::ui::lookandfeel
{

using namespace DesignSystem;

EnhancedThemeManager::EnhancedThemeManager()
{
    initializeColorMap();
}

void EnhancedThemeManager::initializeColorMap()
{
    // Map DesignSystem colors to semantic tokens
    colorMap[ColorToken::BackgroundPrimary] = toColour(Colors::background);
    colorMap[ColorToken::BackgroundSecondary] = toColour(Colors::surface);
    colorMap[ColorToken::BackgroundTertiary] = toColour(Colors::surfaceElevated);
    colorMap[ColorToken::BackgroundElevated] = toColour(Colors::surface4);

    colorMap[ColorToken::SurfacePrimary] = toColour(Colors::surface);
    colorMap[ColorToken::SurfaceSecondary] = toColour(Colors::surface2);
    colorMap[ColorToken::SurfaceTertiary] = toColour(Colors::surface3);
    colorMap[ColorToken::SurfaceElevated] = toColour(Colors::surfaceElevated);

    colorMap[ColorToken::TextPrimary] = toColour(Colors::text);
    colorMap[ColorToken::TextSecondary] = toColour(Colors::textSecondary);
    colorMap[ColorToken::TextTertiary] = toColour(Colors::textTertiary);
    colorMap[ColorToken::TextDisabled] = toColour(Colors::textDisabled);

    colorMap[ColorToken::AccentPrimary] = toColour(Colors::primary);
    colorMap[ColorToken::AccentSecondary] = toColour(Colors::accent);
    colorMap[ColorToken::AccentPositive] = toColour(Colors::success);
    colorMap[ColorToken::AccentNegative] = toColour(Colors::error);
    colorMap[ColorToken::AccentWarning] = toColour(Colors::warning);

    colorMap[ColorToken::BorderPrimary] = toColour(Colors::outline);
    colorMap[ColorToken::BorderFocus] = toColour(Colors::outlineFocus);

    colorMap[ColorToken::InteractivePrimary] = toColour(Colors::primary);
    colorMap[ColorToken::InteractiveHover] = toColour(Colors::primaryHover);
    colorMap[ColorToken::InteractivePressed] = toColour(Colors::primaryPressed);
    colorMap[ColorToken::InteractiveDisabled] = toColour(Colors::textDisabled);

    colorMap[ColorToken::StatusSuccess] = toColour(Colors::success);
    colorMap[ColorToken::StatusError] = toColour(Colors::error);
    colorMap[ColorToken::StatusWarning] = toColour(Colors::warning);
    colorMap[ColorToken::StatusInfo] = toColour(Colors::primary);
}

juce::Colour EnhancedThemeManager::getColor(ColorToken token) const
{
    auto it = colorMap.find(token);
    return it != colorMap.end() ? it->second : juce::Colours::transparentBlack;
}

void EnhancedThemeManager::setColor(ColorToken token, juce::Colour color)
{
    colorMap[token] = color;
    updateColorMap();
}

float EnhancedThemeManager::getSpacing(SpacingToken token) const
{
    switch (token)
    {
        case SpacingToken::Spacing0: return 0.0f;
        case SpacingToken::Spacing1: return static_cast<float>(Spacing::xsmall);
        case SpacingToken::Spacing2: return static_cast<float>(Spacing::small);
        case SpacingToken::Spacing3: return 12.0f;
        case SpacingToken::Spacing4: return static_cast<float>(Spacing::medium);
        case SpacingToken::Spacing5: return 20.0f;
        case SpacingToken::Spacing6: return static_cast<float>(Spacing::large);
        case SpacingToken::Spacing8: return static_cast<float>(Spacing::xlarge);
        case SpacingToken::Spacing10: return 40.0f;
        case SpacingToken::Spacing12: return static_cast<float>(Spacing::xxlarge);
        case SpacingToken::Spacing16: return 64.0f;
        case SpacingToken::Spacing20: return 80.0f;
        default: return static_cast<float>(Spacing::small);
    }
}

juce::Font EnhancedThemeManager::getFont(float size) const
{
    return DesignSystem::getBodyFont(size);
}

juce::Font EnhancedThemeManager::getHeadingFont(float size) const
{
    return DesignSystem::getHeadingFont(size);
}

juce::Font EnhancedThemeManager::getMonoFont(float size) const
{
    return DesignSystem::getMonoFont(size);
}

void EnhancedThemeManager::applyShadow(juce::Graphics& g, int elevation,
                                      juce::Rectangle<float> bounds, float cornerRadius) const
{
    Shadows::ShadowParams params;

    switch (elevation)
    {
        case 0: params = Shadows::elevation0; break;
        case 1: params = Shadows::elevation1; break;
        case 2: params = Shadows::elevation2; break;
        case 3: params = Shadows::elevation3; break;
        case 4: params = Shadows::elevation4; break;
        default: params = Shadows::elevation2; break;
    }

    DesignSystem::applyShadow(g, params, bounds, cornerRadius);
}

void EnhancedThemeManager::updateColorMap()
{
    // Can be extended to notify listeners of theme changes
}

} // namespace daw::ui::lookandfeel

