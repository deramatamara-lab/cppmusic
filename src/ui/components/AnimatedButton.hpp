/**
 * @file AnimatedButton.hpp
 * @brief Enhanced button component with smooth animations
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../animation/Animation.hpp"

namespace cppmusic::ui::components {

/**
 * @brief Button with hover/press animations
 */
class AnimatedButton : public juce::TextButton,
                       public animation::AnimatedComponent {
public:
    AnimatedButton(const juce::String& buttonName = juce::String())
        : juce::TextButton(buttonName)
        , scale_(1.0f)
        , brightness_(1.0f)
    {
    }
    
    void mouseEnter(const juce::MouseEvent& e) override {
        juce::TextButton::mouseEnter(e);
        
        // Slight scale up with back easing for bounce
        scale_.setTarget(1.05f, 150, animation::Easing::easeOutBack);
        brightness_.setTarget(1.1f, 150, animation::Easing::easeOutCubic);
        
        startAnimation([this](float delta) {
            bool s = scale_.update(delta);
            bool b = brightness_.update(delta);
            if (s || b) {
                repaint();
            }
            return s || b;
        });
    }
    
    void mouseExit(const juce::MouseEvent& e) override {
        juce::TextButton::mouseExit(e);
        
        scale_.setTarget(1.0f, 150, animation::Easing::easeOutCubic);
        brightness_.setTarget(1.0f, 150, animation::Easing::easeOutCubic);
        
        startAnimation([this](float delta) {
            bool s = scale_.update(delta);
            bool b = brightness_.update(delta);
            if (s || b) {
                repaint();
            }
            return s || b;
        });
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        juce::TextButton::mouseDown(e);
        
        scale_.setTarget(0.95f, 100, animation::Easing::easeOutCubic);
        
        startAnimation([this](float delta) {
            bool s = scale_.update(delta);
            if (s) {
                repaint();
            }
            return s;
        });
    }
    
    void mouseUp(const juce::MouseEvent& e) override {
        juce::TextButton::mouseUp(e);
        
        if (isMouseOver()) {
            scale_.setTarget(1.05f, 100, animation::Easing::easeOutBack);
        } else {
            scale_.setTarget(1.0f, 100, animation::Easing::easeOutCubic);
        }
        
        startAnimation([this](float delta) {
            bool s = scale_.update(delta);
            if (s) {
                repaint();
            }
            return s;
        });
    }
    
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override {
        auto bounds = getLocalBounds().toFloat();
        auto centre = bounds.getCentre();
        
        // Apply scale transform
        auto transform = juce::AffineTransform::scale(scale_.getValue(), scale_.getValue(), centre.x, centre.y);
        g.addTransform(transform);
        
        // Apply brightness to colors
        auto& lf = getLookAndFeel();
        auto baseColour = findColour(juce::TextButton::buttonColourId);
        auto adjustedColour = baseColour.brighter(brightness_.getValue() - 1.0f);
        
        // Draw with adjusted color
        lf.drawButtonBackground(g, *this, adjustedColour,
                               shouldDrawButtonAsHighlighted,
                               shouldDrawButtonAsDown);
        
        lf.drawButtonText(g, *this, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    }
    
private:
    animation::AnimatedValue<float> scale_;
    animation::AnimatedValue<float> brightness_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnimatedButton)
};

/**
 * @brief Toggle button with smooth state transitions
 */
class AnimatedToggleButton : public juce::ToggleButton,
                             public animation::AnimatedComponent {
public:
    AnimatedToggleButton(const juce::String& buttonText = juce::String())
        : juce::ToggleButton(buttonText)
        , toggleState_(0.0f)
    {
        onClick = [this]() {
            float target = getToggleState() ? 1.0f : 0.0f;
            toggleState_.setTarget(target, 200, animation::Easing::easeOutCubic);
            
            startAnimation([this](float delta) {
                bool isAnimating = toggleState_.update(delta);
                if (isAnimating) {
                    repaint();
                }
                return isAnimating;
            });
        };
    }
    
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override {
        // Use animated state for smooth transitions
        auto t = toggleState_.getValue();
        
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        auto radius = bounds.getHeight() / 2.0f;
        
        // Background
        auto bgColor = juce::Colour(0xFF1F222C).interpolatedWith(
            juce::Colour(0xFFFFA726), t);
        g.setColour(bgColor);
        g.fillRoundedRectangle(bounds, radius);
        
        // Border
        g.setColour(juce::Colour(0xFF303544));
        g.drawRoundedRectangle(bounds, radius, 1.0f);
        
        // Knob
        auto knobX = bounds.getX() + radius + t * (bounds.getWidth() - 2 * radius);
        auto knobBounds = juce::Rectangle<float>(radius * 1.5f, radius * 1.5f)
            .withCentre({knobX, bounds.getCentreY()});
        
        g.setColour(juce::Colours::white);
        g.fillEllipse(knobBounds);
        
        // Text
        auto textBounds = bounds.withTrimmedLeft(bounds.getHeight() + 8);
        g.setFont(14.0f);
        g.setColour(juce::Colour(0xFFE8ECF7));
        g.drawText(getButtonText(), textBounds.toNearestInt(),
                   juce::Justification::centredLeft, true);
    }
    
private:
    animation::AnimatedValue<float> toggleState_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnimatedToggleButton)
};

} // namespace cppmusic::ui::components
