#include "FlagshipDevicePanel.h"
#include "../lookandfeel/MainLookAndFeel.h"

namespace daw::ui::components
{

namespace
{
constexpr std::size_t kMacroCount = 4;
}

FlagshipDevicePanel::FlagshipDevicePanel()
    : title("AI Mastering Suite")
{
    for (std::size_t i = 0; i < kMacroCount; ++i)
    {
        auto slider = std::make_unique<juce::Slider>();
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider->setDoubleClickReturnValue(true, 0.5);
        addAndMakeVisible(slider.get());
        macroSliders[i] = std::move(slider);

        auto label = std::make_unique<juce::Label>();
        label->setJustificationType(juce::Justification::centred);
        label->setText("Macro " + juce::String(static_cast<int>(i + 1)), juce::dontSendNotification);
        addAndMakeVisible(label.get());
        macroLabels[i] = std::move(label);
    }

    startTimerHz(30);
    setInterceptsMouseClicks(true, true);
}

FlagshipDevicePanel::~FlagshipDevicePanel()
{
    stopTimer();
}

void FlagshipDevicePanel::setTitle(const juce::String& newTitle)
{
    if (title != newTitle)
    {
        title = newTitle;
        repaint();
    }
}

void FlagshipDevicePanel::setBackgroundImage(juce::Image newImage)
{
    backgroundImage = std::move(newImage);
    repaint();
}

juce::Slider& FlagshipDevicePanel::getMacroSlider(std::size_t index)
{
    jassert(index < macroSliders.size());
    return *macroSliders[index];
}

void FlagshipDevicePanel::paint(juce::Graphics& g)
{
    const auto& tokens = daw::ui::lookandfeel::getDesignTokens();
    auto bounds = getLocalBounds().toFloat().reduced(6.0f);

    if (auto* lf = dynamic_cast<daw::ui::lookandfeel::MainLookAndFeel*>(&getLookAndFeel()))
    {
        lf->drawPanelBackground(g, bounds);
    }
    else
    {
        g.setColour(tokens.colours.panelBackground);
        g.fillRoundedRectangle(bounds, tokens.radii.large);
    }

    // Hero artwork area
    auto heroArea = bounds.withTrimmedBottom(bounds.getHeight() * 0.45f);
    auto titleArea = heroArea.removeFromTop(36.0f);

    // Title text
    g.setColour(tokens.colours.textPrimary);
    g.setFont(tokens.type.heading());
    g.drawText(title, titleArea, juce::Justification::centredLeft, false);

    // Background art or gradient
    auto artArea = heroArea.reduced(4.0f);
    if (backgroundImage.isValid())
    {
        g.setOpacity(0.9f);
        g.drawImage(backgroundImage, artArea);
        g.setOpacity(1.0f);
    }
    else
    {
        juce::ColourGradient grad(tokens.colours.accentPrimary, artArea.getTopLeft(),
                                  tokens.colours.accentSecondary, artArea.getBottomRight(), false);
        grad.addColour(0.5f, tokens.colours.panelHighlight);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(artArea, tokens.radii.medium);
    }

    // Animated overlay lines to mimic motion
    g.setColour(tokens.colours.accentSecondary.withAlpha(0.25f));
    for (int i = 0; i < 5; ++i)
    {
        const auto offset = std::fmod(animationPhase + static_cast<float>(i) * 0.15f, 1.0f);
        const auto y = artArea.getY() + offset * artArea.getHeight();
        g.drawLine(artArea.getX(), y, artArea.getRight(), y, 1.2f);
    }

    // Macro section background
    auto macroArea = bounds.removeFromBottom(bounds.getHeight() * 0.45f).reduced(8.0f);
    g.setColour(tokens.colours.panelBackground.withAlpha(0.7f));
    g.fillRoundedRectangle(macroArea, tokens.radii.medium);
    g.setColour(tokens.colours.panelBorder.withAlpha(0.4f));
    g.drawRoundedRectangle(macroArea, tokens.radii.medium, 1.0f);
}

void FlagshipDevicePanel::resized()
{
    auto bounds = getLocalBounds().reduced(16);
    auto macroArea = bounds.removeFromBottom(bounds.getHeight() * 0.45f);

    const auto knobWidth = macroArea.getWidth() / static_cast<float>(macroSliders.size());
    for (std::size_t i = 0; i < macroSliders.size(); ++i)
    {
        auto column = macroArea.withWidth(knobWidth).withX(macroArea.getX() + static_cast<float>(i) * knobWidth);
        auto knobBounds = column.withTrimmedTop(10).withTrimmedBottom(34);
        auto labelBounds = column.removeFromBottom(24);

        if (auto* slider = macroSliders[i].get())
            slider->setBounds(knobBounds.reduced(12).toNearestInt());
        if (auto* label = macroLabels[i].get())
            label->setBounds(labelBounds.toNearestInt());
    }
}

void FlagshipDevicePanel::timerCallback()
{
    animationPhase += 0.01f;
    if (animationPhase > 1.0f)
        animationPhase -= 1.0f;
    repaint();
}

} // namespace daw::ui::components
