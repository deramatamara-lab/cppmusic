#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "CustomLookAndFeel.h"
#include "DesignTokens.h"

namespace daw::ui::lookandfeel
{

/**
 * Premium LookAndFeel bound to the DesignTokens theme system.
 */
class MainLookAndFeel final : public CustomLookAndFeel
{
public:
    explicit MainLookAndFeel(Theme theme = Theme::Dark);
    ~MainLookAndFeel() override = default;

    void setTheme(Theme theme);

    void drawPanelBackground(juce::Graphics& g, juce::Rectangle<float> bounds) const;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawLabel(juce::Graphics& g, juce::Label& label) override;

private:
    const DesignTokens* tokens { nullptr };
    Theme currentTheme { Theme::Dark };

    void applyThemeColours();
    void drawOuterGlow(juce::Graphics& g, juce::Rectangle<float> bounds,
                       float radius, float alpha) const;
};

} // namespace daw::ui::lookandfeel
