#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace daw::ui::components
{

/**
 * @brief Lightweight clip launcher visualizer used in the session view.
 * Provides tempo-aware playback highlighting without the legacy dependency.
 */
class ClipLauncherSystem : public juce::Component,
                           private juce::Timer
{
public:
    ClipLauncherSystem();
    ~ClipLauncherSystem() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    void setTempo(double bpm);
    void play();
    void stop();
    void setLoop(bool shouldLoop);

private:
    void timerCallback() override;
    void advanceScene();
    [[nodiscard]] double calculateSceneDurationMs(double bpm) const noexcept;

    double tempoBpm { 128.0 };
    double millisecondsPerScene { 500.0 };
    double lastSceneAdvanceMs { 0.0 };
    double scenePhase { 0.0 };
    bool isPlaying { false };
    bool isLooping { true };
    int numScenes { 4 };
    int clipsPerScene { 4 };
    int activeScene { -1 };
    int queuedScene { -1 };
};

} // namespace daw::ui::components
