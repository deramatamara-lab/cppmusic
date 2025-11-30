#include "NeuroSlider.h"
#include "../../core/ServiceLocator.h"
#include "../animation/AdaptiveAnimationService.h"
#include <cmath>

namespace daw::ui::components
{

using namespace daw::ui::lookandfeel::DesignSystem;

namespace
{
    constexpr float kHoverInMs = 140.0f;
    constexpr float kHoverOutMs = 200.0f;
    constexpr float kGlowInMs = 180.0f;
    constexpr float kGlowOutMs = 260.0f;
    constexpr float kPressGlowMs = 90.0f;
}

NeuroSlider::NeuroSlider(Style style)
    : sliderStyle(style)
{
    setSliderStyle(sliderOrientation == Orientation::Horizontal ?
                   juce::Slider::LinearHorizontal : juce::Slider::LinearVertical);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setLookAndFeel(nullptr);

    if (auto service = daw::core::ServiceLocator::getInstance().getService<animation::AdaptiveAnimationService>())
        animationService = service;
}

NeuroSlider::~NeuroSlider()
{
    cancelAnimation(hoverAnimationId);
    cancelAnimation(glowAnimationId);
}

void NeuroSlider::setStyle(Style newStyle)
{
    sliderStyle = newStyle;
    repaint();
}

void NeuroSlider::setOrientation(Orientation orientation)
{
    sliderOrientation = orientation;
    setSliderStyle(orientation == Orientation::Horizontal ?
                   juce::Slider::LinearHorizontal : juce::Slider::LinearVertical);
    repaint();
}

void NeuroSlider::setValueMapping(const ValueMapping& mapping)
{
    valueMapping = mapping;
    setRange(mapping.minValue, mapping.maxValue, mapping.interval);
    setValue(mapping.defaultValue, juce::dontSendNotification);
}

void NeuroSlider::setAudioReactive(bool enabled, float sensitivity)
{
    audioReactive = enabled;
    audioSensitivity = sensitivity;
}

void NeuroSlider::updateAudioLevel(float level)
{
    audioLevel = juce::jlimit(0.0f, 1.0f, level);
    repaint();
}

void NeuroSlider::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    switch (sliderStyle)
    {
        case Style::Linear:
            paintLinearSlider(g, bounds);
            break;
        case Style::Circular:
            paintCircularSlider(g, bounds);
            break;
        case Style::Arc:
            paintArcSlider(g, bounds);
            break;
        case Style::Waveform:
            paintWaveformSlider(g, bounds);
            break;
        case Style::Spectrum:
            paintSpectrumSlider(g, bounds);
            break;
    }
}

void NeuroSlider::paintLinearSlider(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    constexpr float trackThickness = 4.0f;
    constexpr float thumbSize = 16.0f;
    const auto value = getNormalizedValue();

    juce::Rectangle<float> trackBounds;
    if (sliderOrientation == Orientation::Horizontal)
        trackBounds = { bounds.getX(), bounds.getCentreY() - trackThickness * 0.5f, bounds.getWidth(), trackThickness };
    else
        trackBounds = { bounds.getCentreX() - trackThickness * 0.5f, bounds.getY(), trackThickness, bounds.getHeight() };

    g.setColour(toColour(Colors::surface));
    g.fillRoundedRectangle(trackBounds, Radii::small);

    auto fillBounds = trackBounds;
    if (sliderOrientation == Orientation::Horizontal)
    {
        fillBounds.setWidth(trackBounds.getWidth() * value);
    }
    else
    {
        const float fillHeight = trackBounds.getHeight() * value;
        fillBounds.setY(trackBounds.getBottom() - fillHeight);
        fillBounds.setHeight(fillHeight);
    }

    if (fillBounds.getWidth() > 0.0f && fillBounds.getHeight() > 0.0f)
    {
        auto baseColor = toColour(Colors::primary);
        if (audioReactive && audioLevel > 0.1f)
        {
            const float intensity = juce::jlimit(0.0f, 1.0f, audioLevel * audioSensitivity);
            baseColor = baseColor.brighter(intensity * 0.3f);
        }

        auto accent = baseColor.brighter(glowAmount * 0.2f);
        juce::ColourGradient gradient(accent, fillBounds.getX(), fillBounds.getY(),
                                      accent.darker(0.25f), fillBounds.getRight(), fillBounds.getBottom(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(fillBounds, Radii::small);
    }

    juce::Rectangle<float> thumbBounds;
    if (sliderOrientation == Orientation::Horizontal)
    {
        const float thumbCenterX = trackBounds.getX() + trackBounds.getWidth() * value;
        thumbBounds = { thumbCenterX - thumbSize * 0.5f, bounds.getCentreY() - thumbSize * 0.5f, thumbSize, thumbSize };
    }
    else
    {
        const float thumbCenterY = trackBounds.getBottom() - trackBounds.getHeight() * value;
        thumbBounds = { bounds.getCentreX() - thumbSize * 0.5f, thumbCenterY - thumbSize * 0.5f, thumbSize, thumbSize };
    }

    juce::ColourGradient thumbGradient(toColour(Colors::primary), thumbBounds.getCentreX(), thumbBounds.getY(),
                                       toColour(Colors::primaryDark), thumbBounds.getCentreX(), thumbBounds.getBottom(), false);
    g.setGradientFill(thumbGradient);
    g.fillEllipse(thumbBounds);

    if (glowAmount > 0.01f)
    {
        g.setColour(toColour(Colors::primary).withAlpha(glowAmount * 0.2f));
        g.fillEllipse(thumbBounds.expanded(4.0f + glowAmount * 2.0f));
    }

    if (hoverAmount > 0.01f)
    {
        g.setColour(toColour(Colors::primary).withAlpha(hoverAmount * 0.3f));
        g.drawEllipse(thumbBounds.expanded(2.0f + hoverAmount * 1.5f), 2.0f);
    }
}

void NeuroSlider::paintCircularSlider(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    paintLinearSlider(g, bounds); // Simplified for now
}

void NeuroSlider::paintArcSlider(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    paintLinearSlider(g, bounds); // Simplified for now
}

void NeuroSlider::paintWaveformSlider(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    paintLinearSlider(g, bounds); // Simplified for now
}

void NeuroSlider::paintSpectrumSlider(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    paintLinearSlider(g, bounds); // Simplified for now
}

void NeuroSlider::mouseDown(const juce::MouseEvent& event)
{
    isDragging = true;
    if (onDragStart)
        onDragStart();
    animateState(0.8f, kPressGlowMs, glowAmount, glowAnimationId);
    Slider::mouseDown(event);
}

void NeuroSlider::mouseDrag(const juce::MouseEvent& event)
{
    Slider::mouseDrag(event);
    if (onValueChange)
        onValueChange(getValue());
}

void NeuroSlider::mouseUp(const juce::MouseEvent& event)
{
    isDragging = false;
    if (onDragEnd)
        onDragEnd();
    animateState(isMouseOver ? 0.5f : 0.0f, kGlowOutMs, glowAmount, glowAnimationId);
    Slider::mouseUp(event);
}

void NeuroSlider::mouseDoubleClick(const juce::MouseEvent& event)
{
    setValue(valueMapping.defaultValue, juce::sendNotification);
    Slider::mouseDoubleClick(event);
}

void NeuroSlider::mouseEnter(const juce::MouseEvent& event)
{
    isMouseOver = true;
    animateState(1.0f, kHoverInMs, hoverAmount, hoverAnimationId);
    animateState(0.5f, kGlowInMs, glowAmount, glowAnimationId);
    Slider::mouseEnter(event);
}

void NeuroSlider::mouseExit(const juce::MouseEvent& event)
{
    isMouseOver = false;
    animateState(0.0f, kHoverOutMs, hoverAmount, hoverAnimationId);
    animateState(0.0f, kGlowOutMs, glowAmount, glowAnimationId);
    Slider::mouseExit(event);
}

float NeuroSlider::getNormalizedValue() const
{
    const auto range = getMaximum() - getMinimum();
    if (range <= 0.0)
        return 0.0f;

    const auto raw = static_cast<float>((getValue() - getMinimum()) / range);
    return juce::jlimit(0.0f, 1.0f, raw);
}

void NeuroSlider::animateState(float target, float durationMs, float& storage, uint32_t& handle)
{
    const auto current = storage;
    if (auto service = animationService.lock())
    {
        if (!service->isInitialized())
        {
            storage = target;
            repaint();
            return;
        }

        if (handle != 0)
            service->cancelAnimation(handle);

        auto self = juce::Component::SafePointer<NeuroSlider>(this);
        const auto id = service->animateFloat(current, target, durationMs,
            [self, storagePtr = &storage](float value) {
                if (self != nullptr)
                {
                    *storagePtr = value;
                    self->repaint();
                }
            },
            [self, handlePtr = &handle]() {
                if (self != nullptr)
                    *handlePtr = 0;
            });

        if (id == 0)
        {
            storage = target;
            repaint();
        }
        else
        {
            handle = id;
        }
    }
    else
    {
        storage = target;
        repaint();
    }
}

void NeuroSlider::cancelAnimation(uint32_t& handle)
{
    if (handle == 0)
        return;

    if (auto service = animationService.lock())
        service->cancelAnimation(handle);

    handle = 0;
}

} // namespace daw::ui::components

