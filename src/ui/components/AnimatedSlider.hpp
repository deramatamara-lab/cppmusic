/**
 * @file AnimatedSlider.hpp
 * @brief Enhanced slider component with smooth value animations
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../animation/Animation.hpp"

namespace cppmusic::ui::components {

/**
 * @brief Slider with smooth value interpolation and hover effects
 */
class AnimatedSlider : public juce::Slider,
                       public animation::AnimatedComponent {
public:
    AnimatedSlider(juce::Slider::SliderStyle style = juce::Slider::LinearHorizontal,
                   juce::Slider::TextEntryBoxPosition textBoxPosition = juce::Slider::NoTextBox)
        : juce::Slider(style, textBoxPosition)
        , displayValue_(getValue())
        , hoverGlow_(0.0f)
    {
        onValueChange = [this]() {
            // Smooth interpolation to new value
            displayValue_.setTarget(getValue(), 150, animation::Easing::easeOutCubic);
            
            startAnimation([this](float delta) {
                bool isAnimating = displayValue_.update(delta);
                if (isAnimating) {
                    repaint();
                }
                return isAnimating;
            });
        };
    }
    
    void mouseEnter(const juce::MouseEvent& e) override {
        juce::Slider::mouseEnter(e);
        
        hoverGlow_.setTarget(1.0f, 200, animation::Easing::easeOutCubic);
        
        startAnimation([this](float delta) {
            bool isAnimating = hoverGlow_.update(delta);
            if (isAnimating) {
                repaint();
            }
            return isAnimating;
        });
    }
    
    void mouseExit(const juce::MouseEvent& e) override {
        juce::Slider::mouseExit(e);
        
        hoverGlow_.setTarget(0.0f, 200, animation::Easing::easeOutCubic);
        
        startAnimation([this](float delta) {
            bool isAnimating = hoverGlow_.update(delta);
            if (isAnimating) {
                repaint();
            }
            return isAnimating;
        });
    }
    
    void paint(juce::Graphics& g) override {
        // Draw base slider
        auto& lf = getLookAndFeel();
        
        if (getSliderStyle() == juce::Slider::LinearBar ||
            getSliderStyle() == juce::Slider::LinearBarVertical) {
            lf.drawLinearSlider(g, getLocalBounds().getX(), getLocalBounds().getY(),
                               getLocalBounds().getWidth(), getLocalBounds().getHeight(),
                               getPositionOfValue(displayValue_.getValue()),
                               getPositionOfValue(getMinimum()),
                               getPositionOfValue(getMaximum()),
                               getSliderStyle(), *this);
        } else if (getSliderStyle() == juce::Slider::Rotary ||
                   getSliderStyle() == juce::Slider::RotaryHorizontalDrag ||
                   getSliderStyle() == juce::Slider::RotaryVerticalDrag) {
            lf.drawRotarySlider(g, getLocalBounds().getX(), getLocalBounds().getY(),
                               getLocalBounds().getWidth(), getLocalBounds().getHeight(),
                               valueToProportionOfLength(displayValue_.getValue()),
                               getRotaryParameters().startAngleRadians,
                               getRotaryParameters().endAngleRadians,
                               *this);
        } else {
            lf.drawLinearSlider(g, getLocalBounds().getX(), getLocalBounds().getY(),
                               getLocalBounds().getWidth(), getLocalBounds().getHeight(),
                               getPositionOfValue(displayValue_.getValue()),
                               getPositionOfValue(getMinimum()),
                               getPositionOfValue(getMaximum()),
                               getSliderStyle(), *this);
        }
        
        // Add hover glow
        if (hoverGlow_.getValue() > 0.01f) {
            auto glowColor = juce::Colour(0xFFFFA726).withAlpha(0.3f * hoverGlow_.getValue());
            g.setColour(glowColor);
            g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1), 8.0f);
        }
    }
    
    /**
     * @brief Set value with animation
     */
    void setValueAnimated(double newValue, int durationMs = 300) {
        setValue(newValue);
        displayValue_.setTarget(newValue, durationMs, animation::Easing::easeInOutCubic);
        
        startAnimation([this](float delta) {
            bool isAnimating = displayValue_.update(delta);
            if (isAnimating) {
                repaint();
            }
            return isAnimating;
        });
    }
    
    /**
     * @brief Reset to default value with animation
     */
    void resetToDefault() {
        if (getDoubleClickReturnValue()) {
            setValueAnimated(getDoubleClickReturnValue());
        }
    }
    
private:
    animation::AnimatedValue<double> displayValue_;
    animation::AnimatedValue<float> hoverGlow_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnimatedSlider)
};

/**
 * @brief Rotary knob with enhanced visuals and animations
 */
class AnimatedKnob : public AnimatedSlider {
public:
    AnimatedKnob()
        : AnimatedSlider(juce::Slider::RotaryHorizontalVerticalDrag,
                        juce::Slider::NoTextBox)
    {
        setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                           juce::MathConstants<float>::pi * 2.75f,
                           true);
    }
    
    void mouseDoubleClick(const juce::MouseEvent& e) override {
        juce::Slider::mouseDoubleClick(e);
        resetToDefault();
    }
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnimatedKnob)
};

} // namespace cppmusic::ui::components
