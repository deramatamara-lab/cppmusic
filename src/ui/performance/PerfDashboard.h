/**
 * @file PerfDashboard.h
 * @brief Performance monitoring dashboard header
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace daw::ui::performance {

/**
 * @brief Performance monitoring dashboard
 *
 * Features:
 * - CPU/Memory usage display
 * - Audio thread metrics
 * - Quality tier indicator
 * - Performance history graphs
 */
class PerfDashboard : public juce::Component {
public:
    PerfDashboard();
    ~PerfDashboard() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Metrics update
    void updateMetrics(float cpuUsage, float audioLoad,
                       size_t memUsedMB, size_t memTotalMB);

    // Quality tier (0=Low, 1=Medium, 2=High, 3=Ultra)
    void setQualityTier(int tier);
    [[nodiscard]] int getQualityTier() const;

    // Adaptive mode
    void setAdaptiveModeEnabled(bool enabled);
    [[nodiscard]] bool isAdaptiveModeEnabled() const;

    // Audio settings display
    void setAudioSettings(int bufferSize, double sampleRate);

    // Dropout tracking
    void reportDropout();
    void resetDropoutCount();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerfDashboard)
};

}  // namespace daw::ui::performance
