#include "ClipLauncherSystem.h"

namespace daw::ui::components
{

using daw::ui::lookandfeel::getDesignTokens;

ClipLauncherSystem::ClipLauncherSystem()
{
    tokens = &getDesignTokens();
    millisecondsPerScene = calculateSceneDurationMs(tempoBpm);
    lastSceneAdvanceMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(60);
}

void ClipLauncherSystem::paint(juce::Graphics& g)
{
    const auto background = tokens != nullptr ? tokens->colours.panelBackground : juce::Colours::darkgrey;
    const auto border = tokens != nullptr ? tokens->colours.panelBorder : juce::Colours::black;
    const auto surface = tokens != nullptr ? tokens->colours.backgroundAlt : juce::Colours::dimgrey;
    const auto accent = tokens != nullptr ? tokens->colours.accentPrimary : juce::Colours::orange;

    g.fillAll(background);
    const auto bounds = getLocalBounds().toFloat().reduced(6.0f);
    g.setColour(border);
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

    const auto rowHeight = bounds.getHeight() / static_cast<float>(juce::jmax(1, numScenes));
    const auto columnWidth = bounds.getWidth() / static_cast<float>(juce::jmax(1, clipsPerScene));

    for (int scene = 0; scene < numScenes; ++scene)
    {
        for (int clip = 0; clip < clipsPerScene; ++clip)
        {
            juce::Rectangle<float> cell {
                bounds.getX() + columnWidth * static_cast<float>(clip),
                bounds.getY() + rowHeight * static_cast<float>(scene),
                columnWidth - 4.0f,
                rowHeight - 4.0f
            };

            auto fill = surface;
            if (scene == activeScene)
                fill = accent.withAlpha(0.35f);

            g.setColour(fill);
            g.fillRoundedRectangle(cell, 6.0f);
            g.setColour(border.withAlpha(0.8f));
            g.drawRoundedRectangle(cell, 6.0f, 1.0f);
        }
    }
}

void ClipLauncherSystem::resized()
{
    repaint();
}

void ClipLauncherSystem::setTempo(double bpm)
{
    tempoBpm = juce::jlimit(40.0, 300.0, bpm);
    millisecondsPerScene = calculateSceneDurationMs(tempoBpm);
}

void ClipLauncherSystem::play()
{
    if (isPlaying)
        return;

    isPlaying = true;
    if (activeScene < 0)
        activeScene = 0;
    lastSceneAdvanceMs = juce::Time::getMillisecondCounterHiRes();
    repaint();
}

void ClipLauncherSystem::stop()
{
    if (!isPlaying)
        return;

    isPlaying = false;
    activeScene = -1;
    repaint();
}

void ClipLauncherSystem::setLoop(bool shouldLoop)
{
    isLooping = shouldLoop;
}

void ClipLauncherSystem::timerCallback()
{
    if (!isPlaying)
        return;

    const auto now = juce::Time::getMillisecondCounterHiRes();
    if ((now - lastSceneAdvanceMs) < millisecondsPerScene)
        return;

    lastSceneAdvanceMs = now;
    advanceScene();
}

void ClipLauncherSystem::advanceScene()
{
    if (numScenes <= 0)
        return;

    if (activeScene < 0)
    {
        activeScene = 0;
    }
    else if (activeScene + 1 < numScenes)
    {
        ++activeScene;
    }
    else if (isLooping)
    {
        activeScene = 0;
    }
    else
    {
        activeScene = -1;
        isPlaying = false;
    }

    repaint();
}

double ClipLauncherSystem::calculateSceneDurationMs(double bpm) const noexcept
{
    constexpr double msPerMinute = 60000.0;
    constexpr double beatsPerScene = 4.0; // treat each scene as one bar at 4/4
    const auto safeBpm = juce::jmax(0.001, bpm);
    return (msPerMinute / safeBpm) * beatsPerScene;
}

} // namespace daw::ui::components
