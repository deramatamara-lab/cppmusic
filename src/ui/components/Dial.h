#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"

namespace daw::ui::components
{

/**
 * @brief Neon macro knob with halo
 * 
 * Professional dial/knob component with halo arcs, tick marks, and modulation rings.
 * Follows DAW_DEV_RULES: uses design system, velocity-sensitive dragging.
 */
class Dial : public juce::Slider
{
public:
    Dial();
    ~Dial() override = default;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    /**
     * @brief Set modulation depth visualization (0.0 to 1.0)
     */
    void setModulationDepth(float depth);

    /**
     * @brief Show/hide modulation ring
     */
    void setShowModulationRing(bool show);

private:
    float modulationDepth{0.0f};
    bool showModulationRing{false};
    juce::Point<float> lastMousePosition;
    
    void drawHalo(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void drawKnob(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void drawModulationRing(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    [[nodiscard]] float getValueFromPosition(const juce::Point<int>& pos) const;
};

} // namespace daw::ui::components

