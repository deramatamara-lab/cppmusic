#include "ControlPanel.h"

namespace daw::ui::components
{

ControlPanel::ControlPanel()
    : gainSlider(juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow)
    , gainLabel("gainLabel", "Gain")
    , playButton("Play")
    , stopButton("Stop")
{
    setupControls();
}

void ControlPanel::setupControls()
{
    // Setup gain slider
    gainSlider.setRange(0.0, 1.0, 0.01);
    gainSlider.setValue(0.5);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(gainSlider);
    
    // Setup gain label
    gainLabel.attachToComponent(&gainSlider, false);
    gainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gainLabel);
    
    // Setup buttons
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
}

void ControlPanel::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Fill background
    g.fillAll(juce::Colour(Colors::surface));
    
    // Draw border
    g.setColour(juce::Colour(Colors::primary).withAlpha(0.3f));
    g.drawRect(getLocalBounds(), 1);
    
    // Draw title
    g.setColour(juce::Colour(Colors::text));
    g.setFont(juce::FontOptions().withHeight(static_cast<float>(daw::ui::lookandfeel::DesignSystem::Typography::heading2)));
    g.drawText("Controls", getLocalBounds().removeFromTop(30),
               juce::Justification::centredLeft, false);
}

void ControlPanel::resized()
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    auto bounds = getLocalBounds().reduced(Spacing::medium);
    bounds.removeFromTop(30); // Title area
    
    // Gain slider area
    auto sliderArea = bounds.removeFromTop(120);
    gainSlider.setBounds(sliderArea.reduced(Spacing::small));
    
    // Buttons area
    auto buttonArea = bounds.removeFromTop(40);
    auto buttonWidth = buttonArea.getWidth() / 2 - Spacing::small;
    playButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    stopButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
}

} // namespace daw::ui::components

