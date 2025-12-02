/**
 * @file UIShowcase.hpp
 * @brief Showcase component demonstrating the new UI system
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../animation/Animation.hpp"
#include "../style/LookAndFeel.hpp"
#include "../style/IconManager.hpp"
#include "../style/UIScaleManager.hpp"
#include "AnimatedButton.hpp"
#include "AnimatedSlider.hpp"
#include "TooltipManager.hpp"

namespace cppmusic::ui::components {

/**
 * @brief Showcase component demonstrating animations and styling
 */
class UIShowcase : public juce::Component,
                   public style::UIScaleManager::Listener {
public:
    UIShowcase()
        : animatedButton_("Animated Button")
        , toggleButton_("Toggle Me")
        , slider_()
        , knob_()
    {
        // Setup buttons
        animatedButton_.setTooltip("Click me! (Space)");
        addAndMakeVisible(animatedButton_);
        
        toggleButton_.setTooltip("Toggle me! (T)");
        addAndMakeVisible(toggleButton_);
        
        // Setup slider
        slider_.setRange(0.0, 100.0);
        slider_.setValue(50.0);
        slider_.setTooltip("Drag to adjust (Up/Down)");
        addAndMakeVisible(slider_);
        
        // Setup knob
        knob_.setRange(0.0, 1.0);
        knob_.setValue(0.5);
        knob_.setDoubleClickReturnValue(true, 0.5);
        knob_.setTooltip("Drag to adjust, double-click to reset");
        addAndMakeVisible(knob_);
        
        // Register for scale changes
        style::UIScaleManager::getInstance().addListener(this);
        
        setSize(600, 400);
    }
    
    ~UIShowcase() override {
        style::UIScaleManager::getInstance().removeListener(this);
    }
    
    void paint(juce::Graphics& g) override {
        // Background
        g.fillAll(juce::Colour(0xFF101015));
        
        // Header
        g.setColour(juce::Colour(0xFFE8ECF7));
        g.setFont(24.0f);
        g.drawText("CppMusic UI Showcase", getLocalBounds().removeFromTop(60),
                   juce::Justification::centred, true);
        
        // Labels
        g.setFont(14.0f);
        g.setColour(juce::Colour(0xFFA2A8BC));
        
        auto labelBounds = getLocalBounds().reduced(20);
        labelBounds.removeFromTop(80);
        
        g.drawText("Animated Button:", labelBounds.removeFromTop(60).removeFromTop(20),
                   juce::Justification::left, true);
        labelBounds.removeFromTop(40);
        
        g.drawText("Animated Toggle:", labelBounds.removeFromTop(60).removeFromTop(20),
                   juce::Justification::left, true);
        labelBounds.removeFromTop(40);
        
        g.drawText("Animated Slider:", labelBounds.removeFromTop(60).removeFromTop(20),
                   juce::Justification::left, true);
        labelBounds.removeFromTop(40);
        
        g.drawText("Animated Knob:", labelBounds.removeFromTop(80).removeFromTop(20),
                   juce::Justification::left, true);
        
        // Scale info
        auto scale = style::UIScaleManager::getInstance().getGlobalScale();
        auto scaleText = juce::String("UI Scale: ") + juce::String(scale * 100, 0) + "%";
        g.drawText(scaleText, getLocalBounds().removeFromBottom(30),
                   juce::Justification::centred, true);
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(20);
        bounds.removeFromTop(80);
        
        // Button
        bounds.removeFromTop(20);
        animatedButton_.setBounds(bounds.removeFromTop(40).withWidth(200));
        bounds.removeFromTop(20);
        
        // Toggle
        bounds.removeFromTop(20);
        toggleButton_.setBounds(bounds.removeFromTop(40).withWidth(200));
        bounds.removeFromTop(20);
        
        // Slider
        bounds.removeFromTop(20);
        slider_.setBounds(bounds.removeFromTop(40).withWidth(300));
        bounds.removeFromTop(20);
        
        // Knob
        bounds.removeFromTop(20);
        auto knobBounds = bounds.removeFromTop(80);
        knob_.setBounds(knobBounds.removeFromLeft(80));
    }
    
    void uiScaleChanged(float newScale) override {
        // Respond to scale changes
        resized();
        repaint();
    }
    
private:
    AnimatedButton animatedButton_;
    AnimatedToggleButton toggleButton_;
    AnimatedSlider slider_;
    AnimatedKnob knob_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIShowcase)
};

} // namespace cppmusic::ui::components
