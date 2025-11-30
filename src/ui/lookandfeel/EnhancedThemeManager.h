#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ThemeManager.h"
#include "DesignSystem.h"
#include <unordered_map>

namespace daw::ui::lookandfeel
{

/**
 * EnhancedThemeManager - Extends ThemeManager with comprehensive semantic color system
 *
 * Features:
 * - Semantic color system with comprehensive tokens
 * - Typography scale management
 * - Shadow system with multiple elevations
 * - Spacing system (8px base unit)
 * - Real-time theme switching
 * - Theme inheritance and composition
 */
class EnhancedThemeManager : public ThemeManager
{
public:
    //==============================================================================
    // Semantic Color Tokens
    enum class ColorToken
    {
        // Background colors
        BackgroundPrimary,
        BackgroundSecondary,
        BackgroundTertiary,
        BackgroundElevated,
        BackgroundOverlay,

        // Surface colors
        SurfacePrimary,
        SurfaceSecondary,
        SurfaceTertiary,
        SurfaceElevated,
        SurfaceOverlay,

        // Text colors
        TextPrimary,
        TextSecondary,
        TextTertiary,
        TextDisabled,
        TextInverse,

        // Accent colors
        AccentPrimary,
        AccentSecondary,
        AccentTertiary,
        AccentPositive,
        AccentNegative,
        AccentWarning,
        AccentInfo,

        // Border colors
        BorderPrimary,
        BorderSecondary,
        BorderTertiary,
        BorderFocus,
        BorderError,

        // Interactive colors
        InteractivePrimary,
        InteractiveSecondary,
        InteractiveHover,
        InteractivePressed,
        InteractiveDisabled,

        // Status colors
        StatusSuccess,
        StatusError,
        StatusWarning,
        StatusInfo
    };

    //==============================================================================
    // Spacing Tokens
    enum class SpacingToken
    {
        Spacing0,    // 0px
        Spacing1,    // 4px
        Spacing2,    // 8px
        Spacing3,    // 12px
        Spacing4,    // 16px
        Spacing5,    // 20px
        Spacing6,    // 24px
        Spacing8,    // 32px
        Spacing10,   // 40px
        Spacing12,   // 48px
        Spacing16,   // 64px
        Spacing20    // 80px
    };

    //==============================================================================
    EnhancedThemeManager();
    ~EnhancedThemeManager() = default;

    // Color token access
    juce::Colour getColor(ColorToken token) const;
    void setColor(ColorToken token, juce::Colour color);

    // Spacing access
    float getSpacing(SpacingToken token) const;

    // Typography access
    juce::Font getFont(float size) const;
    juce::Font getHeadingFont(float size) const;
    juce::Font getMonoFont(float size) const;

    // Shadow access
    void applyShadow(juce::Graphics& g, int elevation, juce::Rectangle<float> bounds, float cornerRadius = 0.0f) const;

private:
    std::unordered_map<ColorToken, juce::Colour> colorMap;

    void initializeColorMap();
    void updateColorMap();
};

} // namespace daw::ui::lookandfeel

