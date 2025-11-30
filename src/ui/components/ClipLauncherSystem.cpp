#include "ClipLauncherSystem.h"
#include "../lookandfeel/DesignSystem.h"

namespace daw::ui::components
{

using namespace daw::ui::lookandfeel::DesignSystem;

ClipLauncherSystem::ClipLauncherSystem()
{
    millisecondsPerScene = calculateSceneDurationMs(tempoBpm);
    lastSceneAdvanceMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(60);
}

void ClipLauncherSystem::paint(juce::Graphics& g)
{
    const auto background = juce::Colour(Colors::surface0);
    const auto border      = juce::Colour(Colors::outline);
    const auto accent      = juce::Colour(Colors::accent);

    g.fillAll(background);

    auto bounds = getLocalBounds().toFloat().reduced(static_cast<float>(Spacing::small));
    drawGlassPanel(g, bounds, Radii::medium, true);

    auto gridBounds = bounds.reduced(static_cast<float>(Spacing::small));
    const auto scenes = juce::jmax(1, numScenes);
    const auto clips  = juce::jmax(1, clipsPerScene);

    const auto rowHeight    = gridBounds.getHeight() / static_cast<float>(scenes);
    const auto columnWidth  = gridBounds.getWidth() / static_cast<float>(clips);
    const auto cellSpacing  = static_cast<float>(Spacing::xsmall);
    const auto cellRadius   = Radii::small;
    const auto overlayAlpha = 0.18f;

    for (int scene = 0; scene < scenes; ++scene)
    {
        const bool isActiveScene = (scene == activeScene);
        const bool isQueuedScene = (scene == queuedScene);

        for (int clip = 0; clip < clips; ++clip)
        {
            juce::Rectangle<float> cell(
                gridBounds.getX() + columnWidth * static_cast<float>(clip) + cellSpacing * 0.5f,
                gridBounds.getY() + rowHeight * static_cast<float>(scene) + cellSpacing * 0.5f,
                columnWidth - cellSpacing,
                rowHeight - cellSpacing
            );

            auto baseRowColour = (scene % 2 == 0)
                                   ? juce::Colour(Colors::surface1)
                                   : juce::Colour(Colors::surface2);

            auto fill = baseRowColour;
            if (isActiveScene)
            {
                const auto intensity = static_cast<float>(juce::jlimit(0.0, 1.0, scenePhase));
                auto activeColour = accent.brighter(0.25f * intensity);
                fill = baseRowColour.interpolatedWith(activeColour, 0.55f + 0.35f * intensity);
            }
            else if (isQueuedScene)
            {
                fill = accent.withAlpha(overlayAlpha);
            }

            g.setColour(fill);
            g.fillRoundedRectangle(cell, cellRadius);

            g.setColour(border.withAlpha(isActiveScene ? 0.9f : 0.5f));
            g.drawRoundedRectangle(cell, cellRadius, hairline(this));

            if (isActiveScene && isPlaying && scenePhase > 0.0)
            {
                const auto progress = static_cast<float>(juce::jlimit(0.0, 1.0, scenePhase));
                auto progressBounds = cell;
                progressBounds.setHeight(cell.getHeight() * 0.18f);
                progressBounds.setY(cell.getBottom() - progressBounds.getHeight());
                progressBounds.setWidth(cell.getWidth() * progress);

                g.setColour(accent.withAlpha(0.75f));
                g.fillRoundedRectangle(progressBounds, progressBounds.getHeight() * 0.5f);
            }
        }
    }
}

void ClipLauncherSystem::resized()
{
    repaint();
}

void ClipLauncherSystem::mouseDown(const juce::MouseEvent& event)
{
    if (numScenes <= 0 || clipsPerScene <= 0)
        return;

    auto bounds = getLocalBounds().toFloat().reduced(static_cast<float>(Spacing::small));
    bounds = bounds.reduced(static_cast<float>(Spacing::small));

    auto pos = event.position.toFloat();
    if (!bounds.contains(pos))
        return;

    const auto scenes = juce::jmax(1, numScenes);
    const auto rowHeight = bounds.getHeight() / static_cast<float>(scenes);
    auto relativeY = pos.getY() - bounds.getY();
    auto sceneIndex = juce::jlimit(0, scenes - 1, static_cast<int>(relativeY / rowHeight));

    const auto now = juce::Time::getMillisecondCounterHiRes();

    if (!isPlaying)
    {
        activeScene = sceneIndex;
        queuedScene = -1;
        isPlaying = true;
        lastSceneAdvanceMs = now;
        scenePhase = 0.0;
    }
    else
    {
        queuedScene = sceneIndex;
    }

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
    scenePhase = 0.0;
    repaint();
}

void ClipLauncherSystem::stop()
{
    if (!isPlaying)
        return;

    isPlaying = false;
    activeScene = -1;
    queuedScene = -1;
    scenePhase = 0.0;
    repaint();
}

void ClipLauncherSystem::setLoop(bool shouldLoop)
{
    isLooping = shouldLoop;
}

void ClipLauncherSystem::timerCallback()
{
    if (!isPlaying || millisecondsPerScene <= 0.0)
        return;

    const auto now = juce::Time::getMillisecondCounterHiRes();
    const auto deltaMs = now - lastSceneAdvanceMs;

    scenePhase = juce::jlimit(0.0, 1.0, deltaMs / millisecondsPerScene);

    if (deltaMs >= millisecondsPerScene)
    {
        lastSceneAdvanceMs = now;
        scenePhase = 0.0;

        if (queuedScene >= 0)
        {
            activeScene = queuedScene;
            queuedScene = -1;
        }
        else
        {
            advanceScene();
        }
    }

    repaint();
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
}

double ClipLauncherSystem::calculateSceneDurationMs(double bpm) const noexcept
{
    constexpr double msPerMinute = 60000.0;
    constexpr double beatsPerScene = 4.0; // treat each scene as one bar at 4/4
    const auto safeBpm = juce::jmax(0.001, bpm);
    return (msPerMinute / safeBpm) * beatsPerScene;
}

} // namespace daw::ui::components
