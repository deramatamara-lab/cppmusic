#include "SessionLauncherView.h"
#include "../lookandfeel/DesignSystem.h"

namespace daw::ui::components
{

using namespace daw::ui::lookandfeel::DesignSystem;

SessionLauncherView::SessionLauncherView()
{
    headerLabel.setText("Session Launcher", juce::dontSendNotification);
    headerLabel.setJustificationType(juce::Justification::centredLeft);
    headerLabel.setFont(getHeadingFont(Typography::heading3));
    headerLabel.setColour(juce::Label::textColourId, juce::Colour(Colors::textSecondary));

    addAndMakeVisible(headerLabel);
    addAndMakeVisible(clipLauncher);
}

void SessionLauncherView::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    drawGlassPanel(g, bounds, Radii::medium, true);

    auto headerBounds = headerLabel.getBounds().toFloat();
    auto dividerY = headerBounds.getBottom() + static_cast<float>(Spacing::xsmall) * 0.5f;
    g.setColour(juce::Colour(Colors::divider).withAlpha(0.9f));
    g.fillRect(juce::Rectangle<float>(bounds.getX() + static_cast<float>(Spacing::small),
                                      dividerY,
                                      bounds.getWidth() - static_cast<float>(Spacing::small * 2),
                                      hairline(this)));
}

void SessionLauncherView::resized()
{
    auto area = getLocalBounds().reduced(Spacing::small);
    auto header = area.removeFromTop(24);
    headerLabel.setBounds(header);
    area.removeFromTop(Spacing::xsmall);
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
