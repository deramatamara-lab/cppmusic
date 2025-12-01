#pragma once

#include "../Theme.hpp"
#include <string>
#include <vector>
#include <functional>

namespace daw::ui::imgui
{

/**
 * @brief Condition types for step triggering
 */
enum class StepCondition {
    Always = 0,    ///< Always play
    FirstOnly,     ///< First loop only
    Nth,           ///< Every Nth iteration
    EveryN,        ///< Every N iterations
    SkipM,         ///< Skip first M iterations
    Random         ///< Random based on probability
};

/**
 * @brief Channel state for channel rack with deep-edit capabilities
 */
struct ChannelState
{
    std::string name{"Channel"};
    bool muted{false};
    bool soloed{false};
    float volume{0.8f};
    float pan{0.5f};
    
    // Step sequencer data
    std::vector<bool> steps;           ///< Pattern steps (16 by default)
    std::vector<float> velocities;     ///< Velocity per step [0, 1]
    std::vector<float> probabilities;  ///< Probability per step [0, 1]
    std::vector<StepCondition> conditions; ///< Condition per step
    std::vector<int> conditionParams;  ///< Condition parameter per step
    std::vector<int> microTimingOffsets; ///< Micro-timing offset per step (samples)
    
    // Per-channel parameters
    int transpose{0};           ///< Transpose in semitones (-24 to +24)
    float sampleStartOffset{0.0f}; ///< Sample start offset [0, 1]
    bool reverse{false};        ///< Reverse playback
    float retriggerRate{0.0f};  ///< Retrigger rate (0 = off)
    float channelSwing{0.0f};   ///< Per-channel swing amount [-1, 1]
};

/**
 * @brief Channel Rack panel for pattern-based sequencing
 * 
 * Features:
 * - Step sequencer with velocity/swing/probability lanes
 * - Condition indicators per step
 * - Per-row mute/solo controls
 * - Per-channel params: level, pan, transpose, sample start, reverse, retrigger
 * - Pattern swing per-pattern and per-channel
 * - Flam/roll generator
 */
class ChannelRackPanel
{
public:
    ChannelRackPanel();
    ~ChannelRackPanel() = default;

    /**
     * @brief Draw the channel rack panel
     * @param open Reference to visibility flag
     * @param theme Theme for styling
     */
    void draw(bool& open, const Theme& theme);

    /**
     * @brief Get channels
     */
    [[nodiscard]] std::vector<ChannelState>& getChannels() { return channels_; }

    /**
     * @brief Add a new channel
     */
    void addChannel(const std::string& name = "New Channel");

    /**
     * @brief Set number of steps per pattern
     */
    void setStepsPerPattern(int steps);

    /**
     * @brief Get pattern-level swing amount
     */
    [[nodiscard]] float getPatternSwing() const { return patternSwing_; }
    
    /**
     * @brief Set pattern-level swing amount
     */
    void setPatternSwing(float swing) { patternSwing_ = std::clamp(swing, -1.0f, 1.0f); }

    /**
     * @brief Set callback for step changes
     */
    void setOnStepChanged(std::function<void(int channel, int step, bool active)> callback)
    {
        onStepChanged_ = std::move(callback);
    }

private:
    std::vector<ChannelState> channels_;
    int stepsPerPattern_{16};
    int currentStep_{0};
    bool isDrawMode_{true};  // Draw mode vs select mode
    int selectedChannel_{-1};
    bool showVelocityLane_{true};
    bool showProbabilityLane_{false};
    bool showConditionLane_{false};
    bool showMicroTimingLane_{false};
    float patternSwing_{0.0f};  ///< Pattern-level swing

    std::function<void(int, int, bool)> onStepChanged_;

    void drawChannel(int index, ChannelState& channel, const Theme& theme);
    void drawStepGrid(int channelIndex, ChannelState& channel, const Theme& theme);
    void drawVelocityLane(int channelIndex, ChannelState& channel, const Theme& theme);
    void drawProbabilityLane(int channelIndex, ChannelState& channel, const Theme& theme);
    void drawConditionIndicators(int channelIndex, ChannelState& channel, const Theme& theme);
    void drawChannelParams(int channelIndex, ChannelState& channel, const Theme& theme);
    void createDemoChannels();
    
    // Generators
    void generateFlam(int channelIndex, int step, int flamCount, float flamSpacing);
    void generateRoll(int channelIndex, int startStep, int endStep, int divisions);
};

} // namespace daw::ui::imgui
