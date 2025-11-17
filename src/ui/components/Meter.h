#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"
#include <atomic>

namespace daw::ui::components
{

/**
 * @brief LED meter with gradient
 * 
 * Professional audio meter with gradient fills, peak hold, and multiple zones.
 * Follows DAW_DEV_RULES: real-time safe updates, uses design system.
 */
class Meter : public juce::Component,
              public juce::Timer
{
public:
    enum class Orientation
    {
        Horizontal,
        Vertical
    };

    Meter(Orientation orientation = Orientation::Vertical);
    ~Meter() override = default;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

    /**
     * @brief Set current level (0.0 to 1.0+)
     */
    void setLevel(float level);

    /**
     * @brief Set peak hold enabled
     */
    void setPeakHold(bool enabled);

    /**
     * @brief Reset peak hold
     */
    void resetPeakHold();

    /**
     * @brief Set orientation
     */
    void setOrientation(Orientation orientation);

private:
    Orientation orientation;
    std::atomic<float> currentLevel{0.0f};
    std::atomic<float> peakLevel{0.0f};
    bool peakHoldEnabled{true};
    
    void drawVerticalMeter(juce::Graphics& g);
    void drawHorizontalMeter(juce::Graphics& g);
    [[nodiscard]] juce::Colour getMeterColour(float level) const;
};

} // namespace daw::ui::components

