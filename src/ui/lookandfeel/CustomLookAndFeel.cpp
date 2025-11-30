#include "CustomLookAndFeel.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_graphics/juce_graphics.h>

namespace daw::ui::lookandfeel
{

CustomLookAndFeel::CustomLookAndFeel()
{
    initializeColors();
}

void CustomLookAndFeel::initializeColors()
{
    using namespace DesignSystem;

    // Set color scheme according to design system
    setColour(juce::DocumentWindow::backgroundColourId, juce::Colour(Colors::background));
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(Colors::background));
    setColour(juce::TextButton::buttonColourId, juce::Colour(Colors::primary));
    setColour(juce::TextButton::textColourOffId, juce::Colour(Colors::textSoft));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(Colors::primary));
    setColour(juce::TextButton::textColourOnId, juce::Colour(Colors::textSoft));

    // Text editor colors
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(Colors::surface2));
    setColour(juce::TextEditor::textColourId, juce::Colour(Colors::textSoft));
    setColour(juce::TextEditor::outlineColourId, juce::Colour(Colors::outline));
    setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(Colors::outlineFocus));

    // Combo box colors
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(Colors::surface2));
    setColour(juce::ComboBox::textColourId, juce::Colour(Colors::textSoft));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(Colors::outline));

    // Label colors
    setColour(juce::Label::textColourId, juce::Colour(Colors::textSoft));
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    // Slider colors
    setColour(juce::Slider::backgroundColourId, juce::Colour(Colors::surface2));
    setColour(juce::Slider::thumbColourId, juce::Colour(Colors::primary));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(Colors::primary));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(Colors::outline));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(Colors::textSoft));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(Colors::surface2));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(Colors::outline));
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                          juce::Slider& slider)
{
    using namespace DesignSystem;

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);

    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = radius * 0.12f;  // Slightly thicker for modern look
    auto arcRadius = radius - lineW * 0.5f;

    // Draw shadow for depth
    auto shadowBounds = bounds.expanded(2.0f, 2.0f);
    applyShadow(g, Shadows::elevation1, shadowBounds);

    // Draw background arc with subtle gradient
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(),
                                bounds.getCentreY(),
                                arcRadius,
                                arcRadius,
                                0.0f,
                                rotaryStartAngle,
                                rotaryEndAngle,
                                true);

    // Background with gradient
    juce::ColourGradient bgGradient(juce::Colour(Colors::surface2),
                                    bounds.getCentreX(),
                                    bounds.getCentreY(),
                                    juce::Colour(Colors::surface1),
                                    bounds.getX(),
                                    bounds.getY(),
                                    true);
    g.setGradientFill(bgGradient);
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Draw value arc with gradient
    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(),
                               bounds.getCentreY(),
                               arcRadius,
                               arcRadius,
                               0.0f,
                               rotaryStartAngle,
                               toAngle,
                               true);

        // Primary gradient for value arc
        juce::ColourGradient valueGradient(juce::Colour(Colors::gradientPrimaryStart),
                                           bounds.getCentreX(),
                                           bounds.getCentreY(),
                                           juce::Colour(Colors::gradientPrimaryEnd),
                                           bounds.getX(),
                                           bounds.getY(),
                                           true);
        g.setGradientFill(valueGradient);
        g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Draw thumb with shadow and gradient
    auto thumb = bounds.getCentre().getPointOnCircumference(arcRadius, toAngle);
    auto thumbSize = lineW * 2.5f;
    auto thumbBounds = juce::Rectangle<float>(thumbSize, thumbSize).withCentre(thumb);

    // Thumb shadow
    applyShadow(g, Shadows::elevation1, thumbBounds);

    // Thumb with gradient
    juce::ColourGradient thumbGradient(juce::Colour(Colors::primaryLight),
                                       thumbBounds.getCentreX(),
                                       thumbBounds.getY(),
                                       juce::Colour(Colors::primaryDark),
                                       thumbBounds.getCentreX(),
                                       thumbBounds.getBottom(),
                                       false);
    g.setGradientFill(thumbGradient);
    g.fillEllipse(thumbBounds);

    // Thumb highlight
    g.setColour(juce::Colour(Colors::glassHighlight));
    g.fillEllipse(thumbBounds.reduced(thumbSize * 0.2f));
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                              const juce::Colour& /*backgroundColour*/,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    using namespace DesignSystem;

    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    const auto isToggleOn = button.getToggleState();
    const auto useGradient = true;  // Use gradient for primary buttons

    drawModernButton(g, bounds, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown,
                     isToggleOn, useGradient);
}

void CustomLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float minSliderPos, float maxSliderPos,
                                         const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    using namespace DesignSystem;

    if (style == juce::Slider::LinearVertical || style == juce::Slider::LinearHorizontal)
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        const auto isVertical = (style == juce::Slider::LinearVertical);

        // Draw track background
        auto trackBounds = bounds.reduced(isVertical ? 4.0f : 0.0f, isVertical ? 0.0f : 4.0f);
        g.setColour(juce::Colour(Colors::surface2));
        g.fillRoundedRectangle(trackBounds, Radii::small);

        // Draw track outline
        g.setColour(juce::Colour(Colors::outline));
        g.drawRoundedRectangle(trackBounds, Radii::small, 1.0f);

        // Calculate fill area
        auto fillBounds = trackBounds;
        if (isVertical)
        {
            const auto fillHeight = trackBounds.getHeight() * sliderPos;
            fillBounds.setTop(trackBounds.getBottom() - fillHeight);
        }
        else
        {
            fillBounds.setWidth(trackBounds.getWidth() * sliderPos);
        }

        // Draw fill with gradient
        juce::ColourGradient fillGradient(juce::Colour(Colors::gradientPrimaryStart),
                                         fillBounds.getCentreX(),
                                         fillBounds.getY(),
                                         juce::Colour(Colors::gradientPrimaryEnd),
                                         fillBounds.getCentreX(),
                                         fillBounds.getBottom(),
                                         !isVertical);
        g.setGradientFill(fillGradient);
        g.fillRoundedRectangle(fillBounds, Radii::small);

        // Draw thumb
        auto thumbSize = isVertical ? trackBounds.getWidth() * 0.8f : trackBounds.getHeight() * 0.8f;
        auto thumbBounds = juce::Rectangle<float>(thumbSize, thumbSize);

        if (isVertical)
        {
            thumbBounds.setCentre(trackBounds.getCentreX(), fillBounds.getY());
        }
        else
        {
            thumbBounds.setCentre(fillBounds.getRight(), trackBounds.getCentreY());
        }

        // Thumb shadow
        applyShadow(g, Shadows::elevation1, thumbBounds);

        // Thumb with gradient
        juce::ColourGradient thumbGradient(juce::Colour(Colors::primaryLight),
                                          thumbBounds.getCentreX(),
                                          thumbBounds.getY(),
                                          juce::Colour(Colors::primaryDark),
                                          thumbBounds.getCentreX(),
                                          thumbBounds.getBottom(),
                                          false);
        g.setGradientFill(thumbGradient);
        g.fillEllipse(thumbBounds);

        // Thumb highlight
        g.setColour(juce::Colour(Colors::glassHighlight));
        g.fillEllipse(thumbBounds.reduced(thumbSize * 0.2f));
    }
    else
    {
        // Fallback to default for other styles
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                         minSliderPos, maxSliderPos, style, slider);
    }
}

void CustomLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height,
                                              juce::TextEditor& textEditor)
{
    using namespace DesignSystem;

    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);
    const auto isFocused = textEditor.hasKeyboardFocus(true);

    // Focus outline with enhanced visibility
    if (isFocused)
    {
        g.setColour(juce::Colour(Colors::outlineFocus));
        g.drawRoundedRectangle(bounds, Radii::small, 2.0f);
    }
    else
    {
        // Subtle outline when not focused
        g.setColour(juce::Colour(Colors::outline));
        g.drawRoundedRectangle(bounds, Radii::small, 1.0f);
    }
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, [[maybe_unused]] bool isButtonDown,
                                     [[maybe_unused]] int buttonX, [[maybe_unused]] int buttonY, [[maybe_unused]] int buttonW, [[maybe_unused]] int buttonH,
                                     [[maybe_unused]] juce::ComboBox& box)
{
    using namespace DesignSystem;

    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

    // Glassmorphism background
    drawGlassPanel(g, bounds, Radii::small, false);

    // Draw dropdown arrow
    auto arrowBounds = juce::Rectangle<float>(static_cast<float>(buttonX),
                                              static_cast<float>(buttonY),
                                              static_cast<float>(buttonW),
                                              static_cast<float>(buttonH)).reduced(4.0f);

    juce::Path arrow;
    arrow.addTriangle(arrowBounds.getCentreX(), arrowBounds.getY() + arrowBounds.getHeight() * 0.3f,
                     arrowBounds.getX(), arrowBounds.getBottom() - arrowBounds.getHeight() * 0.3f,
                     arrowBounds.getRight(), arrowBounds.getBottom() - arrowBounds.getHeight() * 0.3f);

    g.setColour(juce::Colour(Colors::textSecondary));
    g.fillPath(arrow);
}

void CustomLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    using namespace DesignSystem;

    // Enhanced label rendering with better typography and contrast
    if (!label.isBeingEdited())
    {
        const auto bounds = label.getLocalBounds().toFloat();
        const auto text = label.getText();

        if (text.isNotEmpty())
        {
            // Get font from label or use design system default
            auto font = label.getFont();
            if (font.getHeight() == 0.0f)
            {
                font = juce::Font(Typography::body, juce::Font::plain);
            }

            // Determine text color based on label state
            auto textColor = juce::Colour(Colors::textSoft);
            if (!label.isEnabled())
            {
                textColor = juce::Colour(Colors::textDisabled);
            }
            else if (label.getAttachedComponent() != nullptr &&
                     label.getAttachedComponent()->hasKeyboardFocus(true))
            {
                textColor = juce::Colour(Colors::outlineFocus);
            }

            // Draw text with subtle shadow for depth (if label is prominent)
            const auto justification = label.getJustificationType();
            const auto isHeading = font.getHeight() >= Typography::heading3;

            if (isHeading)
            {
                // Draw subtle text shadow for headings
                g.setColour(textColor.withAlpha(0.3f));
                g.setFont(font);
                g.drawText(text, bounds.translated(0.0f, 1.0f), justification, false);
            }

            // Draw main text
            g.setColour(textColor);
            g.setFont(font);
            g.drawText(text, bounds, justification, false);
        }
    }
    else
    {
        // Use default drawing when editing
        LookAndFeel_V4::drawLabel(g, label);
    }
}

void CustomLookAndFeel::drawGlassPanel(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                       float cornerRadius, bool elevated) const noexcept
{
    DesignSystem::drawGlassPanel(g, bounds, cornerRadius, elevated);
}

void CustomLookAndFeel::drawModernButton(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                         bool isHighlighted, bool isDown, bool isToggleOn,
                                         bool useGradient) const noexcept
{
    using namespace DesignSystem;

    // Enhanced shadow with elevation based on state
    const auto shadowElevation = (isDown || isToggleOn) ? Shadows::elevation2 : Shadows::elevation1;
    applyShadow(g, shadowElevation, bounds, Radii::medium);

    // Determine button color state with smooth transitions
    juce::Colour baseColor;
    if (isToggleOn)
        baseColor = juce::Colour(Colors::primary);
    else if (isDown)
        baseColor = juce::Colour(Colors::primaryPressed);
    else if (isHighlighted)
        baseColor = juce::Colour(Colors::primaryHover);
    else
        baseColor = juce::Colour(Colors::primary);

    if (useGradient)
    {
        // Enhanced gradient with state-based colors
        juce::Colour startColor = isDown
            ? juce::Colour(Colors::gradientPrimaryEnd)
            : juce::Colour(Colors::gradientPrimaryStart);
        juce::Colour endColor = isDown
            ? juce::Colour(Colors::gradientPrimaryStart)
            : juce::Colour(Colors::gradientPrimaryEnd);

        juce::ColourGradient gradient(startColor,
                                     bounds.getX(),
                                     bounds.getY(),
                                     endColor,
                                     bounds.getX(),
                                     bounds.getBottom(),
                                     false);
        g.setGradientFill(gradient);
    }
    else
    {
        g.setColour(baseColor);
    }

    g.fillRoundedRectangle(bounds, Radii::medium);

    // Enhanced border with state-based opacity
    const auto borderAlpha = isDown ? 0.5f : (isHighlighted ? 0.4f : 0.3f);
    g.setColour(baseColor.withAlpha(borderAlpha));
    g.drawRoundedRectangle(bounds, Radii::medium, isDown ? 1.5f : 1.0f);

    // Enhanced highlight at top with gradient
    auto highlightBounds = bounds;
    highlightBounds.setHeight(bounds.getHeight() * 0.18f);
    juce::ColourGradient highlightGradient(juce::Colour(Colors::glassHighlight),
                                          highlightBounds.getCentreX(),
                                          highlightBounds.getY(),
                                          juce::Colour(Colors::glassHighlight).withAlpha(0.0f),
                                          highlightBounds.getCentreX(),
                                          highlightBounds.getBottom(),
                                          false);
    g.setGradientFill(highlightGradient);
    g.fillRoundedRectangle(highlightBounds, Radii::medium);

    // Subtle inner shadow for pressed state
    if (isDown)
    {
        g.setColour(juce::Colour(0x30000000));
        g.fillRoundedRectangle(bounds.reduced(1.0f), Radii::medium);
    }
}

void CustomLookAndFeel::drawModernMeter(juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                        float level, float peakHold) const noexcept
{
    using namespace DesignSystem;

    // Enhanced background with subtle gradient
    juce::ColourGradient bgGradient(juce::Colour(Colors::meterBackground),
                                    bounds.getCentreX(),
                                    bounds.getY(),
                                    juce::Colour(Colors::meterBackground).darker(0.1f),
                                    bounds.getCentreX(),
                                    bounds.getBottom(),
                                    false);
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(bounds, Radii::small);

    // Calculate fill height
    const auto db = juce::Decibels::gainToDecibels(level);
    const auto normalized = juce::jlimit(0.0f, 1.0f, juce::jmap(db, -60.0f, 0.0f, 0.0f, 1.0f));
    const auto fillHeight = bounds.getHeight() * normalized;

    if (fillHeight > 0.0f)
    {
        auto fillBounds = bounds;
        fillBounds.setTop(bounds.getBottom() - fillHeight);

        // Enhanced gradient with smooth color transitions
        juce::ColourGradient meterGradient(juce::Colour(Colors::meterNormalStart),
                                          fillBounds.getCentreX(),
                                          fillBounds.getBottom(),
                                          juce::Colour(Colors::meterNormalEnd),
                                          fillBounds.getCentreX(),
                                          fillBounds.getY(),
                                          true);

        // Smooth color transitions based on level
        if (db > -3.0f)
        {
            // Danger zone - red gradient
            meterGradient = juce::ColourGradient(juce::Colour(Colors::meterDangerStart),
                                                fillBounds.getCentreX(),
                                                fillBounds.getBottom(),
                                                juce::Colour(Colors::meterDangerEnd),
                                                fillBounds.getCentreX(),
                                                fillBounds.getY(),
                                                true);
        }
        else if (db > -6.0f)
        {
            // Warning zone - yellow gradient
            meterGradient = juce::ColourGradient(juce::Colour(Colors::meterWarningStart),
                                                fillBounds.getCentreX(),
                                                fillBounds.getBottom(),
                                                juce::Colour(Colors::meterWarningEnd),
                                                fillBounds.getCentreX(),
                                                fillBounds.getY(),
                                                true);
        }
        else if (db > -12.0f)
        {
            // Transition zone - green to yellow
            const auto t = juce::jmap(db, -12.0f, -6.0f, 0.0f, 1.0f);
            const auto startColor = juce::Colour(Colors::meterNormalStart).interpolatedWith(
                juce::Colour(Colors::meterWarningStart), t);
            const auto endColor = juce::Colour(Colors::meterNormalEnd).interpolatedWith(
                juce::Colour(Colors::meterWarningEnd), t);
            meterGradient = juce::ColourGradient(startColor,
                                                fillBounds.getCentreX(),
                                                fillBounds.getBottom(),
                                                endColor,
                                                fillBounds.getCentreX(),
                                                fillBounds.getY(),
                                                true);
        }

        g.setGradientFill(meterGradient);
        g.fillRoundedRectangle(fillBounds, Radii::small);

        // Enhanced peak hold indicator with glow effect
        if (peakHold > 0.0f)
        {
            const auto peakDb = juce::Decibels::gainToDecibels(peakHold);
            const auto peakNormalized = juce::jlimit(0.0f, 1.0f, juce::jmap(peakDb, -60.0f, 0.0f, 0.0f, 1.0f));
            const auto peakY = bounds.getBottom() - bounds.getHeight() * peakNormalized;

            // Glow effect
            g.setColour(juce::Colour(Colors::textSoft).withAlpha(0.5f));
            g.drawHorizontalLine(static_cast<int>(peakY - 1.0f), bounds.getX(), bounds.getRight());
            g.setColour(juce::Colour(Colors::textSoft));
            g.drawHorizontalLine(static_cast<int>(peakY), bounds.getX(), bounds.getRight());
            g.setColour(juce::Colour(Colors::textSoft).withAlpha(0.5f));
            g.drawHorizontalLine(static_cast<int>(peakY + 1.0f), bounds.getX(), bounds.getRight());
        }
    }

    // Enhanced border with subtle shadow
    g.setColour(juce::Colour(Colors::outline));
    g.drawRoundedRectangle(bounds, Radii::small, 1.0f);

    // Subtle inner highlight at top
    auto highlightBounds = bounds;
    highlightBounds.setHeight(bounds.getHeight() * 0.05f);
    g.setColour(juce::Colour(Colors::glassHighlight).withAlpha(0.3f));
    g.fillRoundedRectangle(highlightBounds, Radii::small);
}

} // namespace daw::ui::lookandfeel

