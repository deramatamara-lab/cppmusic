#include "SessionLauncherView.h"

namespace daw::ui::components
{

using daw::ui::lookandfeel::getDesignTokens;

SessionLauncherView::SessionLauncherView()
{
    tokens = &getDesignTokens();
    headerLabel.setText("Session Launcher", juce::dontSendNotification);
    headerLabel.setJustificationType(juce::Justification::centredLeft);
    if (tokens != nullptr)
    {
        headerLabel.setFont(tokens->type.title());
        headerLabel.setColour(juce::Label::textColourId, tokens->colours.textSecondary);
    }
    addAndMakeVisible(headerLabel);
    addAndMakeVisible(clipLauncher);
}

void SessionLauncherView::paint(juce::Graphics& g)
{
    if (tokens != nullptr)
    {
        g.fillAll(tokens->colours.backgroundAlt);
        g.setColour(tokens->colours.panelBorder.withAlpha(0.5f));
        g.drawRect(getLocalBounds().toFloat(), 1.0f);
    }
    else
        g.fillAll(juce::Colours::black);
}

void SessionLauncherView::resized()
{
    auto area = getLocalBounds().reduced(10);
    auto header = area.removeFromTop(30);
    headerLabel.setBounds(header);
    area.removeFromTop(4);
    clipLauncher.setBounds(area);
}

void SessionLauncherView::setTempo(double bpm)
{
    clipLauncher.setTempo(bpm);
}

void SessionLauncherView::setIsPlaying(bool isPlaying)
{
    if (isPlaying)
        clipLauncher.play();
    else
        clipLauncher.stop();
}

void SessionLauncherView::setLooping(bool shouldLoop)
{
    clipLauncher.setLoop(shouldLoop);
}

} // namespace daw::ui::components
