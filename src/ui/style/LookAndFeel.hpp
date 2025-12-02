/**
 * @file LookAndFeel.hpp
 * @brief CppMusic DAW Look and Feel implementation.
 *
 * Provides a premium, NI/iZotope-grade aesthetic for the DAW UI.
 * Defines:
 * - Base color palette (dark background, high-contrast accents)
 * - Typography (custom sans-serif for labels, mono for values)
 * - Standard JUCE drawing overrides
 * - Uniform rounding, shadows, and gradients
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace cppmusic::ui::style {

/**
 * @brief Color palette for the DAW UI
 *
 * Premium dark theme inspired by FL Studio, NI, and iZotope products.
 */
struct ColorPalette {
    // Background tones (charcoal / blue-grey)
    juce::Colour backgroundDark{0xFF101015};     ///< Deepest background
    juce::Colour backgroundMid{0xFF161821};      ///< Surface 1
    juce::Colour backgroundLight{0xFF1F222C};    ///< Surface 2
    
    // Panel and border
    juce::Colour panelBorder{0xFF303544};        ///< Panel borders
    juce::Colour panelShadow{0x59000000};        ///< Soft shadow
    
    // Text colors
    juce::Colour textPrimary{0xFFE8ECF7};        ///< Primary text
    juce::Colour textSecondary{0xFFA2A8BC};      ///< Secondary/dimmed text
    juce::Colour textDisabled{0xFF5A6070};       ///< Disabled state text
    
    // Accent colors
    juce::Colour accentPrimary{0xFFFFA726};      ///< FL-style orange accent
    juce::Colour accentSecondary{0xFF4ADE80};    ///< Neon green
    juce::Colour accentTertiary{0xFF00D4FF};     ///< Cyan
    juce::Colour focus{0xFF00D4FF};              ///< Focus indicator (accessibility)
    
    // Status colors
    juce::Colour warning{0xFFFFB020};            ///< Warning yellow
    juce::Colour danger{0xFFFF4D4D};             ///< Error/danger red
    juce::Colour success{0xFF22D39B};            ///< Success green
    
    // Meter colors
    juce::Colour meterOK{0xFF22D39B};            ///< Normal level
    juce::Colour meterHot{0xFFFFC857};           ///< Hot level
    juce::Colour meterClip{0xFFFF4D4D};          ///< Clipping
    
    // Grid colors
    juce::Colour gridMain{0xFF2A2F3A};           ///< Main grid lines
    juce::Colour gridSubtle{0xFF1C2029};         ///< Subtle grid lines
};

/**
 * @brief Font types for the DAW UI
 */
enum class FontType {
    Body,       ///< Default UI text
    Label,      ///< Slider labels, button text
    Title,      ///< Section headers
    Monospace   ///< Time readouts, diagnostic text
};

/**
 * @brief Typography settings for the DAW UI
 */
struct Typography {
    juce::String fontFamily{"Inter"};            ///< Primary font family
    juce::String fontFamilyMono{"JetBrains Mono"};///< Monospace font
    
    float size12{12.0f};
    float size14{14.0f};
    float size16{16.0f};
    float size18{18.0f};
    float size24{24.0f};
    float size32{32.0f};
};

/**
 * @brief Spacing and radius values
 */
struct Spacing {
    int s2{2};
    int s4{4};
    int s6{6};
    int s8{8};
    int s12{12};
    int s16{16};
    int s24{24};
    int s32{32};
    
    float radiusSmall{8.0f};
    float radiusMedium{12.0f};
    float radiusLarge{16.0f};
    float radiusXL{22.0f};
};

/**
 * @brief CppMusic DAW Look and Feel
 *
 * Premium look and feel for a professional DAW aesthetic.
 * Extends LookAndFeel_V4 with custom drawing for:
 * - Buttons (gradient backgrounds, glow effects)
 * - Sliders (rotary and linear with modern styling)
 * - ComboBoxes (dark theme)
 * - Toggle buttons
 * - Scrollbars
 * - Table headers
 */
class CppMusicLookAndFeel : public juce::LookAndFeel_V4 {
public:
    CppMusicLookAndFeel();
    ~CppMusicLookAndFeel() override = default;

    // =========================================================================
    // Color Palette Access
    // =========================================================================

    [[nodiscard]] const ColorPalette& getColors() const noexcept { return colors_; }
    [[nodiscard]] const Typography& getTypography() const noexcept { return typography_; }
    [[nodiscard]] const Spacing& getSpacing() const noexcept { return spacing_; }

    void setColorPalette(const ColorPalette& palette);
    void setTypography(const Typography& typography);
    
    /**
     * @brief Get font by type with optional size override
     * @param type Font type
     * @param size Optional size override (if 0, uses default for type)
     */
    juce::Font getFont(FontType type, float size = 0.0f) const;

    // =========================================================================
    // Typography Overrides
    // =========================================================================

    juce::Font getLabelFont(juce::Label& label) override;
    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;
    juce::Font getComboBoxFont(juce::ComboBox& box) override;
    juce::Font getPopupMenuFont() override;

    // =========================================================================
    // Button Drawing
    // =========================================================================

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    // =========================================================================
    // Slider Drawing
    // =========================================================================

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    // =========================================================================
    // ComboBox Drawing
    // =========================================================================

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    // =========================================================================
    // Scrollbar Drawing
    // =========================================================================

    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
                       int x, int y, int width, int height,
                       bool isScrollbarVertical, int thumbStartPosition,
                       int thumbSize, bool isMouseOver, bool isMouseDown) override;

    // =========================================================================
    // Table Header Drawing
    // =========================================================================

    void drawTableHeaderBackground(juce::Graphics& g,
                                    juce::TableHeaderComponent& header) override;

    void drawTableHeaderColumn(juce::Graphics& g, juce::TableHeaderComponent& header,
                                const juce::String& columnName, int columnId,
                                int width, int height, bool isMouseOver,
                                bool isMouseDown, int columnFlags) override;

    // =========================================================================
    // Label Drawing
    // =========================================================================

    void drawLabel(juce::Graphics& g, juce::Label& label) override;

private:
    ColorPalette colors_;
    Typography typography_;
    Spacing spacing_;

    void applyColorsToLookAndFeel();
    
    // Cached fonts
    mutable juce::Font labelFont_;
    mutable juce::Font buttonFont_;
    mutable juce::Font comboFont_;
    mutable juce::Font monoFont_;
    
    void updateFonts();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CppMusicLookAndFeel)
};

} // namespace cppmusic::ui::style
