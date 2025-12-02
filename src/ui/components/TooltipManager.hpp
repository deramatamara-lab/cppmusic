/**
 * @file TooltipManager.hpp
 * @brief Professional tooltip system with animations and keyboard shortcuts
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../animation/Animation.hpp"
#include <memory>

namespace cppmusic::ui::components {

/**
 * @brief Enhanced tooltip window with fade animations
 */
class AnimatedTooltipWindow : public juce::Component,
                              public animation::AnimatedComponent,
                              private juce::Timer {
public:
    AnimatedTooltipWindow()
        : opacity_(0.0f)
    {
        setAlwaysOnTop(true);
        setOpaque(false);
    }
    
    void displayTip(const juce::Point<int>& position, const juce::String& text,
                   const juce::String& shortcut = juce::String()) {
        if (text.isEmpty()) {
            hide();
            return;
        }
        
        tooltipText_ = text;
        shortcutText_ = shortcut;
        
        // Calculate size
        juce::Font font(14.0f);
        auto textWidth = font.getStringWidth(text);
        auto shortcutWidth = shortcut.isNotEmpty() ? font.getStringWidth(shortcut) + 16 : 0;
        auto width = textWidth + shortcutWidth + 24;
        auto height = 32;
        
        // Position tooltip
        auto screenBounds = juce::Desktop::getInstance().getDisplays()
            .getDisplayForPoint(position)->userArea;
        auto x = position.x + 10;
        auto y = position.y + 20;
        
        // Keep on screen
        if (x + width > screenBounds.getRight()) {
            x = position.x - width - 10;
        }
        if (y + height > screenBounds.getBottom()) {
            y = position.y - height - 10;
        }
        
        setBounds(x, y, width, height);
        
        // Add to desktop first for proper positioning
        addToDesktop(juce::ComponentPeer::windowIsTemporary |
                    juce::ComponentPeer::windowIgnoresKeyPresses);
        
        // Fade in
        opacity_.setTarget(1.0f, 150, animation::Easing::easeOutCubic);
        startAnimation([this](float delta) {
            bool isAnimating = opacity_.update(delta);
            if (isAnimating) {
                repaint();
            }
            return isAnimating;
        });
        
        setVisible(true);
        
        // Auto-hide after 5 seconds
        startTimer(5000);
    }
    
    void hide() {
        stopTimer();
        
        opacity_.setTarget(0.0f, 150, animation::Easing::easeOutCubic);
        startAnimation([this](float delta) {
            bool isAnimating = opacity_.update(delta);
            if (isAnimating) {
                repaint();
            } else {
                setVisible(false);
            }
            return isAnimating;
        });
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Shadow
        g.setColour(juce::Colour(0x59000000).withAlpha(opacity_.getValue()));
        g.fillRoundedRectangle(bounds.translated(0, 2).reduced(-2), 6.0f);
        
        // Background
        g.setColour(juce::Colour(0xFF1F222C).withAlpha(opacity_.getValue()));
        g.fillRoundedRectangle(bounds, 6.0f);
        
        // Border
        g.setColour(juce::Colour(0xFF303544).withAlpha(opacity_.getValue()));
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
        
        // Text
        g.setColour(juce::Colour(0xFFE8ECF7).withAlpha(opacity_.getValue()));
        g.setFont(14.0f);
        
        auto textBounds = bounds.reduced(12, 0);
        if (shortcutText_.isNotEmpty()) {
            auto shortcutWidth = g.getCurrentFont().getStringWidth(shortcutText_) + 16;
            auto mainTextBounds = textBounds.withTrimmedRight(shortcutWidth);
            auto shortcutBounds = textBounds.withTrimmedLeft(textBounds.getWidth() - shortcutWidth);
            
            g.drawText(tooltipText_, mainTextBounds.toNearestInt(),
                      juce::Justification::centredLeft, true);
            
            // Shortcut badge
            g.setColour(juce::Colour(0xFF303544).withAlpha(opacity_.getValue()));
            g.fillRoundedRectangle(shortcutBounds.reduced(0, 4), 3.0f);
            
            g.setColour(juce::Colour(0xFFA2A8BC).withAlpha(opacity_.getValue()));
            g.setFont(12.0f);
            g.drawText(shortcutText_, shortcutBounds.toNearestInt(),
                      juce::Justification::centred, true);
        } else {
            g.drawText(tooltipText_, textBounds.toNearestInt(),
                      juce::Justification::centredLeft, true);
        }
    }
    
private:
    void timerCallback() override {
        hide();
    }
    
    juce::String tooltipText_;
    juce::String shortcutText_;
    animation::AnimatedValue<float> opacity_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnimatedTooltipWindow)
};

/**
 * @brief Enhanced tooltip client with keyboard shortcuts
 */
class TooltipManager : public juce::TooltipClient {
public:
    /**
     * @brief Set tooltip with optional keyboard shortcut
     */
    void setTooltipWithShortcut(const juce::String& text, const juce::String& shortcut = juce::String()) {
        tooltipText_ = text;
        shortcutText_ = shortcut;
    }
    
    juce::String getTooltip() override {
        if (shortcutText_.isNotEmpty()) {
            return tooltipText_ + " (" + shortcutText_ + ")";
        }
        return tooltipText_;
    }
    
protected:
    juce::String tooltipText_;
    juce::String shortcutText_;
};

} // namespace cppmusic::ui::components
