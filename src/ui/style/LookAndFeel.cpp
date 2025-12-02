/**
 * @file LookAndFeel.cpp
 * @brief Implementation of CppMusic DAW Look and Feel.
 */

#include "LookAndFeel.hpp"
#include <cmath>

namespace cppmusic::ui::style {

CppMusicLookAndFeel::CppMusicLookAndFeel() {
    updateFonts();
    applyColorsToLookAndFeel();
}

void CppMusicLookAndFeel::setColorPalette(const ColorPalette& palette) {
    colors_ = palette;
    applyColorsToLookAndFeel();
}

void CppMusicLookAndFeel::setTypography(const Typography& typography) {
    typography_ = typography;
    updateFonts();
}

void CppMusicLookAndFeel::updateFonts() {
    // Use system default fonts as fallbacks if custom fonts aren't available
    auto sansSerifFamily = typography_.fontFamily;
    auto monoFamily = typography_.fontFamilyMono;
    
    // Check if custom font is available, fall back to system defaults
    juce::Font testFont(sansSerifFamily, 12.0f, juce::Font::plain);
    if (testFont.getTypefaceName() != sansSerifFamily) {
        sansSerifFamily = juce::Font::getDefaultSansSerifFontName();
    }
    
    juce::Font testMonoFont(monoFamily, 12.0f, juce::Font::plain);
    if (testMonoFont.getTypefaceName() != monoFamily) {
        monoFamily = juce::Font::getDefaultMonospacedFontName();
    }
    
    labelFont_ = juce::Font(sansSerifFamily, typography_.size14, juce::Font::plain);
    buttonFont_ = juce::Font(sansSerifFamily, typography_.size14, juce::Font::plain);
    comboFont_ = juce::Font(sansSerifFamily, typography_.size14, juce::Font::plain);
    monoFont_ = juce::Font(monoFamily, typography_.size14, juce::Font::plain);
}

void CppMusicLookAndFeel::applyColorsToLookAndFeel() {
    // Apply colors to standard JUCE color IDs
    setColour(juce::DocumentWindow::backgroundColourId, colors_.backgroundDark);
    setColour(juce::ResizableWindow::backgroundColourId, colors_.backgroundDark);
    
    setColour(juce::TextButton::buttonColourId, colors_.accentPrimary);
    setColour(juce::TextButton::buttonOnColourId, colors_.accentSecondary);
    setColour(juce::TextButton::textColourOffId, colors_.textPrimary);
    setColour(juce::TextButton::textColourOnId, colors_.textPrimary);
    
    setColour(juce::Slider::thumbColourId, colors_.accentPrimary);
    setColour(juce::Slider::trackColourId, colors_.accentSecondary.withAlpha(0.3f));
    setColour(juce::Slider::rotarySliderFillColourId, colors_.accentPrimary);
    setColour(juce::Slider::rotarySliderOutlineColourId, colors_.gridMain);
    
    setColour(juce::Label::textColourId, colors_.textPrimary);
    setColour(juce::Label::outlineColourId, colors_.panelBorder);
    
    setColour(juce::PopupMenu::backgroundColourId, colors_.backgroundMid);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, colors_.accentSecondary.withAlpha(0.2f));
    setColour(juce::PopupMenu::textColourId, colors_.textPrimary);
    setColour(juce::PopupMenu::highlightedTextColourId, colors_.textPrimary);
    
    setColour(juce::ComboBox::backgroundColourId, colors_.backgroundLight);
    setColour(juce::ComboBox::outlineColourId, colors_.panelBorder);
    setColour(juce::ComboBox::textColourId, colors_.textPrimary);
    
    setColour(juce::ScrollBar::thumbColourId, colors_.accentPrimary.withAlpha(0.5f));
    setColour(juce::ScrollBar::trackColourId, colors_.backgroundDark);
    
    setColour(juce::TableHeaderComponent::backgroundColourId, colors_.backgroundMid);
    setColour(juce::TableHeaderComponent::textColourId, colors_.textSecondary);
    setColour(juce::TableHeaderComponent::outlineColourId, colors_.panelBorder);
}

// =========================================================================
// Typography Overrides
// =========================================================================

juce::Font CppMusicLookAndFeel::getLabelFont(juce::Label& /*label*/) {
    return labelFont_;
}

juce::Font CppMusicLookAndFeel::getTextButtonFont(juce::TextButton& /*button*/, 
                                                   int buttonHeight) {
    return buttonFont_.withHeight(buttonHeight > 0 ? 
        juce::jmin(static_cast<float>(buttonHeight) * 0.6f, 18.0f) : 14.0f);
}

juce::Font CppMusicLookAndFeel::getComboBoxFont(juce::ComboBox& /*box*/) {
    return comboFont_;
}

juce::Font CppMusicLookAndFeel::getPopupMenuFont() {
    return labelFont_;
}

juce::Font CppMusicLookAndFeel::getFont(FontType type, float size) const {
    juce::Font font;
    float defaultSize = typography_.size14;
    
    switch (type) {
        case FontType::Body:
            font = labelFont_;
            defaultSize = typography_.size14;
            break;
        case FontType::Label:
            font = buttonFont_;
            defaultSize = typography_.size14;
            break;
        case FontType::Title:
            font = labelFont_;
            defaultSize = typography_.size18;
            break;
        case FontType::Monospace:
            font = monoFont_;
            defaultSize = typography_.size14;
            break;
    }
    
    return size > 0.0f ? font.withHeight(size) : font.withHeight(defaultSize);
}

// =========================================================================
// Button Drawing
// =========================================================================

void CppMusicLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                const juce::Colour& /*backgroundColour*/,
                                                bool shouldDrawButtonAsHighlighted,
                                                bool shouldDrawButtonAsDown) {
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto radius = spacing_.radiusMedium;
    
    auto base = colors_.accentPrimary;
    if (shouldDrawButtonAsDown) {
        base = base.darker(0.3f);
    } else if (shouldDrawButtonAsHighlighted) {
        base = base.brighter(0.1f);
    }
    
    // Gradient background
    juce::ColourGradient grad(base.brighter(0.2f), bounds.getTopLeft(),
                               base.darker(0.4f), bounds.getBottomRight(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds, radius);
    
    // Border glow on hover/down
    if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown) {
        g.setColour(colors_.accentSecondary.withAlpha(0.6f));
        g.drawRoundedRectangle(bounds, radius, 1.5f);
        
        // Outer glow
        auto glowBounds = bounds.expanded(2.0f);
        g.setColour(colors_.accentPrimary.withAlpha(0.2f));
        g.drawRoundedRectangle(glowBounds, radius + 2.0f, 1.0f);
    }
}

void CppMusicLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown) {
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    auto radius = bounds.getHeight() / 2.0f;
    
    auto isOn = button.getToggleState();
    auto base = isOn ? colors_.accentPrimary : colors_.backgroundLight;
    
    if (shouldDrawButtonAsHighlighted) {
        base = base.brighter(0.15f);
    }
    if (shouldDrawButtonAsDown) {
        base = base.brighter(0.25f);
    }
    
    g.setColour(base);
    g.fillRoundedRectangle(bounds, radius);
    
    g.setColour(colors_.panelBorder);
    g.drawRoundedRectangle(bounds, radius, 1.0f);
    
    // Button text
    g.setFont(buttonFont_);
    g.setColour(isOn ? colors_.backgroundDark : colors_.textPrimary);
    g.drawText(button.getButtonText(), bounds.toNearestInt(), 
               juce::Justification::centred, true);
}

// =========================================================================
// Slider Drawing
// =========================================================================

void CppMusicLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, 
                                            int width, int height,
                                            float sliderPos, float rotaryStartAngle,
                                            float rotaryEndAngle, juce::Slider& /*slider*/) {
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(6.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centre = bounds.getCentre();
    auto knobRadius = radius * 0.75f;
    
    // Background halo
    g.setColour(colors_.accentPrimary.withAlpha(0.08f));
    g.fillEllipse(bounds);
    
    // Outer halo arc
    juce::Path haloArc;
    haloArc.addCentredArc(centre.x, centre.y, radius - 2.0f, radius - 2.0f, 
                          0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(colors_.accentPrimary.withAlpha(0.15f));
    g.strokePath(haloArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    
    // Knob body
    auto knobArea = juce::Rectangle<float>(knobRadius * 2.0f, knobRadius * 2.0f)
        .withCentre(centre);
    juce::ColourGradient knobGrad(colors_.backgroundLight.brighter(0.25f), knobArea.getTopLeft(),
                                   colors_.backgroundDark.darker(0.3f), knobArea.getBottomRight(), 
                                   false);
    g.setGradientFill(knobGrad);
    g.fillEllipse(knobArea);
    
    // Knob border
    g.setColour(colors_.panelBorder.withAlpha(0.8f));
    g.drawEllipse(knobArea, 1.1f);
    
    // Value arc
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto arcRadius = knobRadius + 6.0f;
    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 
                           0.0f, rotaryStartAngle, angle, true);
    
    juce::ColourGradient valueGrad(colors_.accentPrimary, 
        centre.getPointOnCircumference(arcRadius, rotaryStartAngle),
        colors_.accentSecondary, 
        centre.getPointOnCircumference(arcRadius, angle), 
        false);
    g.setGradientFill(valueGrad);
    g.strokePath(valueArc, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    
    // Pointer
    auto pointerRadius = knobRadius * 0.8f;
    juce::Point<float> pointer(
        centre.x + std::cos(angle) * pointerRadius,
        centre.y + std::sin(angle) * pointerRadius
    );
    g.setColour(colors_.accentSecondary.withAlpha(0.5f));
    g.drawLine(centre.x, centre.y, pointer.x, pointer.y, 3.0f);
    g.setColour(colors_.accentSecondary);
    g.drawLine(centre.x, centre.y, pointer.x, pointer.y, 2.0f);
}

void CppMusicLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, 
                                            int width, int height,
                                            float sliderPos, float minSliderPos, 
                                            float maxSliderPos,
                                            juce::Slider::SliderStyle style, 
                                            juce::Slider& slider) {
    juce::ignoreUnused(minSliderPos, maxSliderPos);
    
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    bool isVertical = (style == juce::Slider::LinearVertical ||
                       style == juce::Slider::LinearBarVertical);
    
    // Track
    auto trackBounds = isVertical ?
        bounds.withSizeKeepingCentre(4.0f, bounds.getHeight()) :
        bounds.withSizeKeepingCentre(bounds.getWidth(), 4.0f);
    
    g.setColour(colors_.gridMain);
    g.fillRoundedRectangle(trackBounds, 2.0f);
    
    // Filled portion
    auto fillBounds = trackBounds;
    if (isVertical) {
        auto fillHeight = trackBounds.getHeight() * sliderPos;
        fillBounds = trackBounds.removeFromBottom(fillHeight);
    } else {
        auto fillWidth = trackBounds.getWidth() * sliderPos;
        fillBounds = trackBounds.removeFromLeft(fillWidth);
    }
    
    juce::ColourGradient fillGrad(colors_.accentPrimary, fillBounds.getTopLeft(),
                                   colors_.accentSecondary, fillBounds.getBottomRight(), 
                                   false);
    g.setGradientFill(fillGrad);
    g.fillRoundedRectangle(fillBounds, 2.0f);
    
    // Thumb
    float thumbSize = isVertical ? 
        juce::jmin(14.0f, bounds.getWidth() * 0.8f) :
        juce::jmin(14.0f, bounds.getHeight() * 0.8f);
    
    juce::Point<float> thumbPos;
    if (isVertical) {
        auto thumbY = bounds.getBottom() - bounds.getHeight() * sliderPos;
        thumbPos = { bounds.getCentreX(), thumbY };
    } else {
        auto thumbX = bounds.getX() + bounds.getWidth() * sliderPos;
        thumbPos = { thumbX, bounds.getCentreY() };
    }
    
    auto thumbBounds = juce::Rectangle<float>(thumbSize, thumbSize).withCentre(thumbPos);
    
    // Thumb glow
    g.setColour(colors_.accentPrimary.withAlpha(0.3f));
    g.fillEllipse(thumbBounds.expanded(3.0f));
    
    // Thumb body
    g.setColour(slider.isMouseOverOrDragging() ? 
        colors_.accentPrimary.brighter(0.2f) : colors_.accentPrimary);
    g.fillEllipse(thumbBounds);
    
    // Thumb border
    g.setColour(colors_.accentSecondary);
    g.drawEllipse(thumbBounds, 1.5f);
}

// =========================================================================
// ComboBox Drawing
// =========================================================================

void CppMusicLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, 
                                        bool /*isButtonDown*/,
                                        int buttonX, int buttonY, int buttonW, int buttonH,
                                        juce::ComboBox& box) {
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);
    
    // Background
    juce::ColourGradient gradient(colors_.backgroundMid, bounds.getTopLeft(),
                                   colors_.backgroundLight, bounds.getBottomRight(), 
                                   false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, spacing_.radiusLarge);
    
    // Border
    g.setColour(colors_.panelBorder);
    g.drawRoundedRectangle(bounds, spacing_.radiusLarge, 1.0f);
    
    // Arrow
    juce::Path arrow;
    auto arrowBounds = juce::Rectangle<float>(
        static_cast<float>(buttonX), static_cast<float>(buttonY),
        static_cast<float>(buttonW), static_cast<float>(buttonH)
    ).reduced(4.0f);
    arrow.addTriangle(
        arrowBounds.getCentreX(), arrowBounds.getBottom() - arrowBounds.getHeight() * 0.25f,
        arrowBounds.getX(), arrowBounds.getY() + arrowBounds.getHeight() * 0.25f,
        arrowBounds.getRight(), arrowBounds.getY() + arrowBounds.getHeight() * 0.25f
    );
    g.setColour(colors_.textSecondary);
    g.fillPath(arrow);
    
    box.setColour(juce::ComboBox::textColourId, colors_.textPrimary);
}

// =========================================================================
// Scrollbar Drawing
// =========================================================================

void CppMusicLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
                                         int x, int y, int width, int height,
                                         bool isScrollbarVertical, int thumbStartPosition,
                                         int thumbSize, bool isMouseOver, bool isMouseDown) {
    juce::ignoreUnused(scrollbar);
    
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    
    // Track
    g.setColour(colors_.backgroundDark);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Thumb
    juce::Rectangle<float> thumbBounds;
    if (isScrollbarVertical) {
        thumbBounds = juce::Rectangle<float>(
            bounds.getX() + 2, 
            static_cast<float>(thumbStartPosition),
            bounds.getWidth() - 4, 
            static_cast<float>(thumbSize)
        );
    } else {
        thumbBounds = juce::Rectangle<float>(
            static_cast<float>(thumbStartPosition), 
            bounds.getY() + 2,
            static_cast<float>(thumbSize), 
            bounds.getHeight() - 4
        );
    }
    
    auto thumbColor = isMouseDown ? colors_.accentPrimary :
                      isMouseOver ? colors_.accentPrimary.withAlpha(0.7f) :
                                    colors_.accentPrimary.withAlpha(0.5f);
    g.setColour(thumbColor);
    g.fillRoundedRectangle(thumbBounds, 3.0f);
}

// =========================================================================
// Table Header Drawing
// =========================================================================

void CppMusicLookAndFeel::drawTableHeaderBackground(juce::Graphics& g,
                                                     juce::TableHeaderComponent& header) {
    auto bounds = header.getLocalBounds().toFloat();
    g.setColour(colors_.backgroundMid);
    g.fillRect(bounds);
    g.setColour(colors_.panelBorder);
    g.drawHorizontalLine(static_cast<int>(bounds.getBottom() - 1), 
                         bounds.getX(), bounds.getRight());
}

void CppMusicLookAndFeel::drawTableHeaderColumn(juce::Graphics& g, 
                                                 juce::TableHeaderComponent& /*header*/,
                                                 const juce::String& columnName, int /*columnId*/,
                                                 int width, int height, bool isMouseOver,
                                                 bool isMouseDown, int /*columnFlags*/) {
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
    
    if (isMouseDown) {
        g.setColour(colors_.accentSecondary.withAlpha(0.2f));
        g.fillRect(bounds);
    } else if (isMouseOver) {
        g.setColour(colors_.accentSecondary.withAlpha(0.1f));
        g.fillRect(bounds);
    }
    
    g.setColour(colors_.textSecondary);
    g.setFont(labelFont_);
    g.drawText(columnName, bounds.reduced(4), juce::Justification::centredLeft, true);
    
    g.setColour(colors_.panelBorder);
    g.drawVerticalLine(width - 1, bounds.getY(), bounds.getBottom());
}

// =========================================================================
// Label Drawing
// =========================================================================

void CppMusicLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label) {
    if (label.isBeingEdited()) {
        LookAndFeel_V4::drawLabel(g, label);
        return;
    }
    
    g.setFont(getLabelFont(label));
    g.setColour(label.isEnabled() ? colors_.textPrimary : colors_.textDisabled);
    g.drawFittedText(label.getText(), label.getLocalBounds().toNearestInt(), 
                     label.getJustificationType(), 1);
}

} // namespace cppmusic::ui::style
