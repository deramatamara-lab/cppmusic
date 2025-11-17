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
    auto base = t.colours.accentPrimary;
    if (shouldDrawButtonAsDown)
        base = base.darker(0.1f);
    else if (shouldDrawButtonAsHighlighted)
        base = base.brighter(0.15f);

    if (!isToggle)
    {
        juce::ColourGradient grad(t.colours.accentPrimary.brighter(0.2f),
                                   bounds.getTopLeft(),
                                   t.colours.accentPrimary.darker(0.4f),
                                   bounds.getBottomRight(),
                                   false);
        g.setGradientFill(grad);
    }
    else
    {
        g.setColour(base);
    }

    g.fillRoundedRectangle(bounds, radius);

    g.setColour(base.withAlpha(isToggle ? 0.9f : 0.6f));
    g.drawRoundedRectangle(bounds, radius, 1.4f);

    if (shouldDrawButtonAsHighlighted || isToggle)
    {
        auto highlight = bounds.reduced(2.0f);
        g.setColour(t.colours.accentSecondary.withAlpha(0.25f));
        g.drawRoundedRectangle(highlight, radius - 1.0f, 1.2f);
    }

    drawOuterGlow(g, bounds, t.elevation.controlShadowRadius,
                  t.elevation.controlShadowAlpha * (isToggle ? 1.2f : 1.0f));
}

void MainLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                       juce::Slider& /*slider*/)
{
    jassert(tokens != nullptr);
    const auto& t = *tokens;
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(6.0f);
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre = bounds.getCentre();
    const auto knobRadius = radius * 0.75f;

    g.setColour(t.colours.accentPrimary.withAlpha(0.08f));
    g.fillEllipse(bounds);

    auto knobArea = juce::Rectangle<float>(knobRadius * 2.0f, knobRadius * 2.0f).withCentre(centre);
    juce::ColourGradient knobGrad(t.colours.panelHighlight.brighter(0.25f), knobArea.getTopLeft(),
                                  t.colours.panelBackground.darker(0.3f), knobArea.getBottomRight(), false);
    g.setGradientFill(knobGrad);
    g.fillEllipse(knobArea);

    g.setColour(t.colours.panelBorder.withAlpha(0.8f));
    g.drawEllipse(knobArea, 1.1f);

    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto arcRadius = knobRadius + 6.0f;
    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
    g.setColour(t.colours.accentPrimary);
    g.strokePath(valueArc, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto pointerRadius = knobRadius * 0.7f;
    juce::Point<float> pointer(
        centre.x + std::cos(angle) * pointerRadius,
        centre.y + std::sin(angle) * pointerRadius);
    g.setColour(t.colours.accentSecondary);
    g.drawLine(centre.x, centre.y, pointer.x, pointer.y, 2.0f);

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
