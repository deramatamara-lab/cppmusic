#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "DesignSystem.h"

namespace daw::ui::lookandfeel
{

/**
 * @brief Custom LookAndFeel implementation
 * 
 * Provides consistent styling across all UI components.
 * Follows the design system from development rules.
 */
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();
    ~CustomLookAndFeel() override = default;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;
    
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override;
    
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
    
    
    void drawTextEditorOutline(juce::Graphics& g, int width, int height,
                                juce::TextEditor& textEditor) override;
    
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;
    
    void drawLabel(juce::Graphics& g, juce::Label& label) override;
    
    // Public rendering helpers for components
    void drawModernMeter(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                         float level, float peakHold) const noexcept;

private:
    void initializeColors();
    
    // Glassmorphism and modern rendering helpers
    void drawGlassPanel(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                        float cornerRadius, bool elevated) const noexcept;
    
    void drawModernButton(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                          bool isHighlighted, bool isDown, bool isToggleOn,
                          bool useGradient) const noexcept;
};

} // namespace daw::ui::lookandfeel

