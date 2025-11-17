#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"
#include "../../project/Pattern.h"
#include <vector>
#include <memory>

namespace daw::ui::components
{

/**
 * @brief FL-style step sequencer
 * 
 * Advanced step sequencer with probability, micro-timing, and trig conditions.
 * Follows DAW_DEV_RULES: professional UX, 60fps performance.
 */
class StepSequencer : public juce::Component,
                      public juce::Timer
{
public:
    struct StepData
    {
        bool active{false};
        uint8_t velocity{127};
        float probability{1.0f};
        float microTiming{0.0f}; // -1.0 to 1.0, in samples
        int trigCondition{0}; // 0 = always, 1 = every 2, 2 = every 4, etc.
    };

    StepSequencer();
    ~StepSequencer() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void setTempo(double bpm);
    void play();
    void stop();

    /**
     * @brief Set number of steps
     */
    void setNumSteps(int numSteps);

    /**
     * @brief Get number of steps
     */
    [[nodiscard]] int getNumSteps() const noexcept { return static_cast<int>(steps.size()); }

    /**
     * @brief Set step data
     */
    void setStep(int stepIndex, const StepData& data);

    /**
     * @brief Get step data
     */
    [[nodiscard]] StepData getStep(int stepIndex) const;

    /**
     * @brief Set current play position
     */
    void setPlayPosition(int step);

    /**
     * @brief Set pattern to edit
     */
    void setPattern(std::shared_ptr<daw::project::Pattern> pattern);

private:
    void timerCallback() override;
    [[nodiscard]] double calculateStepDurationMs(double bpm) const noexcept;

    std::vector<StepData> steps;
    int numSteps{16};
    int currentPlayPosition{-1};
    std::shared_ptr<daw::project::Pattern> pattern;
    double tempoBpm{120.0};
    double millisecondsPerStep{125.0};
    double lastStepAdvanceTimeMs{0.0};
    bool isPlaying{false};
    
    float stepWidth{30.0f};
    float stepHeight{30.0f};
    
    void updateFromPattern();
    void updatePattern();
    [[nodiscard]] int getStepAtPosition(const juce::Point<int>& pos) const;
};

} // namespace daw::ui::components

