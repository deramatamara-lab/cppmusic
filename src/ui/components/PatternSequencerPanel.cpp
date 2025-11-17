#include "PatternSequencerPanel.h"

namespace daw::ui::components
{

using daw::ui::lookandfeel::getDesignTokens;

PatternSequencerPanel::PatternSequencerPanel()
{
    tokens = &getDesignTokens();
    headerLabel.setText("Pattern Sequencer", juce::dontSendNotification);
    headerLabel.setJustificationType(juce::Justification::centredLeft);
    if (tokens != nullptr)
    {
        headerLabel.setFont(tokens->type.title());
        headerLabel.setColour(juce::Label::textColourId, tokens->colours.textSecondary);
    }
    addAndMakeVisible(headerLabel);
    addAndMakeVisible(stepSequencer);
}

void PatternSequencerPanel::paint(juce::Graphics& g)
{
    if (tokens != nullptr)
    {
        g.fillAll(tokens->colours.panelBackground);
        auto bounds = getLocalBounds().toFloat();
        g.setColour(tokens->colours.panelBorder.withAlpha(0.4f));
        g.drawRect(bounds, 1.0f);
    }
    else
        g.fillAll(juce::Colours::black);
}

void PatternSequencerPanel::resized()
{
    auto area = getLocalBounds().reduced(10);
    auto header = area.removeFromTop(30);
    headerLabel.setBounds(header);
    area.removeFromTop(4);
    stepSequencer.setBounds(area);
}

void PatternSequencerPanel::setTempo(double bpm)
{
    stepSequencer.setTempo(bpm);
}

void PatternSequencerPanel::setIsPlaying(bool isPlaying)
{
    if (isPlaying)
        stepSequencer.play();
    else
        stepSequencer.stop();
}

} // namespace daw::ui::components
