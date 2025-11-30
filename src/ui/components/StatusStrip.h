#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../audio/engine/EngineContext.h"
#include "../lookandfeel/DesignSystem.h"
#include <memory>

namespace daw::ui::components
{

/**
 * @brief Status strip component
 *
 * Displays CPU%, XRuns count, RAM usage (MB), project name, and sample rate.
 * Updates via Timer (30-60 Hz throttled).
 * Follows DAW_DEV_RULES: uses design system, no allocations in paint().
 */
class StatusStrip : public juce::Component, public juce::Timer
{
public:
    explicit StatusStrip(std::shared_ptr<daw::audio::engine::EngineContext> engineContext);
    ~StatusStrip() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Set project name (called from MainView when project changes)
    void setProjectName(const juce::String& name) { projectName = name; repaint(); }

    static constexpr int kStatusStripHeight = 24;

private:
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;

    juce::String projectName;
    float cpuLoadPercent = 0.0f;
    uint64_t xrunCount = 0;
    float ramUsageMB = 0.0f;
    double sampleRate = 44100.0;

    // Update throttling (30-60 Hz)
    static constexpr int kUpdateIntervalMs = 33; // ~30 Hz

    void updateMetrics();
    juce::Colour getCpuColor(float percent) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusStrip)
};

} // namespace daw::ui::components

