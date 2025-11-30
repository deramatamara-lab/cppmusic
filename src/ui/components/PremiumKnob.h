#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../core/PhysicsAnimation.h"

namespace daw::ui::components
{

/**
 * PremiumKnob - iZotope/Native Instruments Style
 * Ultra-smooth animations, advanced visual feedback, professional styling
 */
class PremiumKnob : public juce::Slider, public juce::Timer
{
public:
    PremiumKnob();
    ~PremiumKnob() override;

    void timerCallback() override;

    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    // Premium knob styles
    enum class KnobStyle
    {
        Classic,        // Classic circular knob
        Modern,         // Modern flat knob with ring
        Spectrum,       // Spectrum analyzer style
        Vintage,        // Vintage analog style
        Futuristic      // Futuristic glass style
    };

    void setKnobStyle(KnobStyle style) { knobStyle = style; repaint(); }
    void setAudioReactive(bool reactive) { audioReactive = reactive; }
    void updateAudioLevel(float level) { audioLevel = level; repaint(); }

private:
    KnobStyle knobStyle = KnobStyle::Modern;
    bool audioReactive = false;
    float audioLevel = 0.0f;
    daw::ui::core::PhysicsAnimation hoverProgress = daw::ui::core::PhysicsAnimation::smooth();
    daw::ui::core::PhysicsAnimation glowIntensity = daw::ui::core::PhysicsAnimation::gentle();

    void paintClassicKnob(juce::Graphics& g, juce::Rectangle<float> bounds);
    void paintModernKnob(juce::Graphics& g, juce::Rectangle<float> bounds);
    void paintSpectrumKnob(juce::Graphics& g, juce::Rectangle<float> bounds);
    void paintVintageKnob(juce::Graphics& g, juce::Rectangle<float> bounds);
    void paintFuturisticKnob(juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PremiumKnob)
};

} // namespace daw::ui::components

