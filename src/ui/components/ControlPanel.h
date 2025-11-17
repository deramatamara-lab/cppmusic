#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"

namespace daw::ui::components
{

/**
 * @brief Control panel component
 */
class ControlPanel : public juce::Component
{
public:
    ControlPanel();
    ~ControlPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Slider gainSlider;
    juce::Label gainLabel;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    
    void setupControls();
};

} // namespace daw::ui::components

