#include "MainLookAndFeel.h"

#include <cmath>

namespace daw::ui::lookandfeel
{

MainLookAndFeel::MainLookAndFeel(Theme theme)
{
    setTheme(theme);
}

void MainLookAndFeel::setTheme(Theme theme)
{
    if (tokens != nullptr && currentTheme == theme)
        return;

    currentTheme = theme;
    tokens = &getDesignTokens(theme);
    applyThemeColours();
}

void MainLookAndFeel::applyThemeColours()
{
    jassert(tokens != nullptr);
    if (tokens == nullptr)
        return;

    const auto& t = *tokens;
    setColour(juce::DocumentWindow::backgroundColourId, t.colours.background);
    setColour(juce::ResizableWindow::backgroundColourId, t.colours.background);
    setColour(juce::TextButton::buttonColourId, t.colours.accentPrimary);
    setColour(juce::TextButton::textColourOffId, t.colours.textPrimary);
    setColour(juce::TextButton::textColourOnId, t.colours.textPrimary);
    setColour(juce::Slider::thumbColourId, t.colours.accentPrimary);
    setColour(juce::Slider::trackColourId, t.colours.panelHighlight);
    setColour(juce::Label::textColourId, t.colours.textPrimary);
}

void MainLookAndFeel::drawPanelBackground(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    jassert(tokens != nullptr);
    const auto& t = *tokens;
    const auto radius = t.radii.large;
    juce::Path panelPath;
    panelPath.addRoundedRectangle(bounds, radius);

    juce::ColourGradient gradient(t.colours.panelBackground,
                                  bounds.getTopLeft(),
                                  t.colours.panelHighlight,
                                  bounds.getBottomRight(),
                                  false);
    gradient.addColour(0.5f, t.colours.panelBackground.interpolatedWith(t.colours.panelHighlight, 0.4f));

    g.setGradientFill(gradient);
    g.fillPath(panelPath);

    g.setColour(t.colours.panelBorder);
    g.strokePath(panelPath, juce::PathStrokeType(1.0f));

    drawOuterGlow(g, bounds, t.elevation.panelShadowRadius, t.elevation.panelShadowAlpha);
}

void MainLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                           const juce::Colour& /*backgroundColour*/,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    jassert(tokens != nullptr);
    const auto& t = *tokens;
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const auto radius = t.radii.medium;

    const auto isToggle = button.getToggleState();
    const auto isActive = isToggle || shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown;

    // Base color with state variations
    auto base = t.colours.accentPrimary;
    if (shouldDrawButtonAsDown)
        base = t.colours.accentPrimaryActive;
    else if (shouldDrawButtonAsHighlighted)
        base = t.colours.accentPrimaryHover;

    // Gradient fill for all buttons
    juce::ColourGradient grad(base.brighter(0.2f),
                               bounds.getTopLeft(),
                               base.darker(0.4f),
                               bounds.getBottomRight(),
                               false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds, radius);

    // Neon glow outline - stronger when active/hovered
    const auto glowAlpha = isActive ? 0.9f : 0.6f;
    g.setColour(t.colours.accentSecondary.withAlpha(glowAlpha));
    g.drawRoundedRectangle(bounds, radius, isActive ? 1.8f : 1.4f);

    // Inner highlight ring for active/hovered states
    if (isActive)
    {
        auto highlight = bounds.reduced(2.0f);
        // Neon cyan glow
        g.setColour(t.colours.accentSecondary.withAlpha(0.4f));
        g.drawRoundedRectangle(highlight, radius - 1.0f, 1.5f);

        // Additional inner glow
        auto innerGlow = bounds.reduced(3.0f);
        g.setColour(t.colours.accentSecondary.withAlpha(0.15f));
        g.drawRoundedRectangle(innerGlow, radius - 2.0f, 1.0f);
    }

    // Outer glow effect - enhanced for active buttons
    const auto glowIntensity = isActive ? 1.5f : 1.0f;
    drawOuterGlow(g, bounds, t.elevation.controlShadowRadius,
                  t.elevation.controlShadowAlpha * glowIntensity);
}

void MainLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                       juce::Slider& slider)
{
    juce::ignoreUnused(slider);
    jassert(tokens != nullptr);
    const auto& t = *tokens;
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(6.0f);
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre = bounds.getCentre();
    const auto knobRadius = radius * 0.75f;

    // Background glow halo
    g.setColour(t.colours.accentPrimary.withAlpha(0.08f));
    g.fillEllipse(bounds);

    // Outer halo arc (modulation ring) - shows full range
    const auto haloRadius = radius - 2.0f;
    juce::Path haloArc;
    haloArc.addCentredArc(centre.x, centre.y, haloRadius, haloRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(t.colours.accentPrimary.withAlpha(0.15f));
    g.strokePath(haloArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Tick marks on halo
    constexpr int numTicks = 12;
    g.setColour(t.colours.accentPrimary.withAlpha(0.3f));
    for (int i = 0; i <= numTicks; ++i)
    {
        const auto tickAngle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * (static_cast<float>(i) / static_cast<float>(numTicks));
        const auto tickStart = centre.getPointOnCircumference(haloRadius - 2.0f, tickAngle);
        const auto tickEnd = centre.getPointOnCircumference(haloRadius + 2.0f, tickAngle);
        g.drawLine(tickStart.x, tickStart.y, tickEnd.x, tickEnd.y, 1.0f);
    }

    // Knob body with gradient
    auto knobArea = juce::Rectangle<float>(knobRadius * 2.0f, knobRadius * 2.0f).withCentre(centre);
    juce::ColourGradient knobGrad(t.colours.panelHighlight.brighter(0.25f), knobArea.getTopLeft(),
                                  t.colours.panelBackground.darker(0.3f), knobArea.getBottomRight(), false);
    g.setGradientFill(knobGrad);
    g.fillEllipse(knobArea);

    // Knob border
    g.setColour(t.colours.panelBorder.withAlpha(0.8f));
    g.drawEllipse(knobArea, 1.1f);

    // Value arc with neon gradient
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto arcRadius = knobRadius + 6.0f;
    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);

    // Neon gradient for value arc
    juce::ColourGradient valueGrad(t.colours.accentPrimary,
                                   centre.getPointOnCircumference(arcRadius, rotaryStartAngle),
                                   t.colours.accentSecondary,
                                   centre.getPointOnCircumference(arcRadius, angle),
                                   false);
    g.setGradientFill(valueGrad);
    g.strokePath(valueArc, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer needle with neon accent
    const auto pointerRadius = knobRadius * 0.7f;
    juce::Point<float> pointer(
        centre.x + std::cos(angle) * pointerRadius,
        centre.y + std::sin(angle) * pointerRadius);

    // Pointer with glow effect
    g.setColour(t.colours.accentSecondary.withAlpha(0.5f));
    g.drawLine(centre.x, centre.y, pointer.x, pointer.y, 3.0f);
    g.setColour(t.colours.accentSecondary);
    g.drawLine(centre.x, centre.y, pointer.x, pointer.y, 2.0f);

    // Outer glow for knob
    drawOuterGlow(g, knobArea, t.elevation.controlShadowRadius, t.elevation.controlShadowAlpha);
}

void MainLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                   int buttonX, int buttonY, int buttonW, int buttonH,
                                   juce::ComboBox& box)
{
    jassert(tokens != nullptr);
    const auto& t = *tokens;
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);
    drawPanelBackground(g, bounds);

    juce::Path arrow;
    auto arrowBounds = juce::Rectangle<float>(static_cast<float>(buttonX), static_cast<float>(buttonY),
                                              static_cast<float>(buttonW), static_cast<float>(buttonH)).reduced(4.0f);
    arrow.addTriangle(arrowBounds.getCentreX(), arrowBounds.getBottom() - arrowBounds.getHeight() * 0.25f,
                      arrowBounds.getX(), arrowBounds.getY() + arrowBounds.getHeight() * 0.25f,
                      arrowBounds.getRight(), arrowBounds.getY() + arrowBounds.getHeight() * 0.25f);
    g.setColour(t.colours.textSecondary);
    g.fillPath(arrow);

    box.setColour(juce::ComboBox::textColourId, t.colours.textPrimary);
}

void MainLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    if (label.isBeingEdited())
    {
        CustomLookAndFeel::drawLabel(g, label);
        return;
    }

    jassert(tokens != nullptr);
    const auto& t = *tokens;
    const auto bounds = label.getLocalBounds().toFloat();
    g.setFont(label.getFont().getHeight() > 0.0f ? label.getFont() : t.type.body());
    g.setColour(label.isEnabled() ? t.colours.textPrimary : t.colours.textDisabled);
    g.drawFittedText(label.getText(), bounds.toNearestInt(), label.getJustificationType(), 1);
}

void MainLookAndFeel::drawOuterGlow(juce::Graphics& g, juce::Rectangle<float> bounds,
                                    float radius, float alpha) const
{
    jassert(tokens != nullptr);
    const auto& t = *tokens;
    juce::Path glow;
    glow.addRoundedRectangle(bounds.expanded(2.0f), t.radii.large + 4.0f);
    g.setColour(t.colours.accentPrimary.withAlpha(alpha));
    g.strokePath(glow, juce::PathStrokeType(radius * 0.05f));
}

} // namespace daw::ui::lookandfeel
