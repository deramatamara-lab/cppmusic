#pragma once

#include "../Theme.hpp"
#include <string>
#include <vector>
#include <functional>
#include <array>

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
    Random,        ///< Random based on probability
    Fill,          ///< Play only during fill
    NotFill        ///< Play except during fill
};

/**
 * @brief Channel type
 */
enum class ChannelType {
    Sampler,       ///< Sample-based channel
    Synth,         ///< Synthesizer plugin
    Audio,         ///< Audio track
    Automation,    ///< Automation clip channel
    Layer          ///< Layer (triggers other channels)
};

/**
 * @brief Graph editor target
 */
enum class GraphTarget {
    Velocity,
    Pan,
    Pitch,
    Filter,
    Modulation,
    Volume
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

    // Channel type and color
    ChannelType type{ChannelType::Sampler};
    ImVec4 color{0.3f, 0.5f, 0.7f, 1.0f};

    // Step sequencer data
    std::vector<bool> steps;           ///< Pattern steps (16 by default)
    std::vector<float> velocities;     ///< Velocity per step [0, 1]
    std::vector<float> probabilities;  ///< Probability per step [0, 1]
    std::vector<StepCondition> conditions; ///< Condition per step
    std::vector<int> conditionParams;  ///< Condition parameter per step
    std::vector<int> microTimingOffsets; ///< Micro-timing offset per step (samples)

    // Graph editor data (FL-style per-step graphs)
    std::vector<float> panValues;      ///< Pan per step [-1, 1]
    std::vector<float> pitchValues;    ///< Pitch per step in semitones
    std::vector<float> filterValues;   ///< Filter cutoff per step [0, 1]
    std::vector<float> modValues;      ///< Modulation per step [0, 1]

    // Per-channel parameters
    int transpose{0};           ///< Transpose in semitones (-24 to +24)
    float sampleStartOffset{0.0f}; ///< Sample start offset [0, 1]
    bool reverse{false};        ///< Reverse playback
    float retriggerRate{0.0f};  ///< Retrigger rate (0 = off)
    float channelSwing{0.0f};   ///< Per-channel swing amount [-1, 1]

    // FL-style channel properties
    int targetMixerTrack{0};    ///< Target mixer track (0 = master)
    int rootNote{60};           ///< Root note for pitch (C5)
    float fineTune{0.0f};       ///< Fine tune in cents
    bool cut{false};            ///< Cut self
    int cutBy{0};               ///< Cut by group (0 = none)

    // Layer settings
    std::vector<int> layerTargets;  ///< Channels triggered by this layer

    // Plugin reference
    int pluginId{-1};           ///< Plugin instance ID
    std::string pluginName;

    // Loop settings
    bool loopEnabled{false};
    float loopStart{0.0f};
    float loopEnd{1.0f};
};

/**
 * @brief Fill pattern for fill tool
 */
enum class FillPattern {
    All,           ///< Fill all steps
    Half,          ///< Every other step
    Quarter,       ///< Every 4th step
    Eighth,        ///< Every 8th step
    Custom,        ///< Custom euclidean pattern
    Euclidean      ///< Euclidean rhythm
};

/**
 * @brief Channel Rack panel for pattern-based sequencing (FL Studio style)
 *
 * Features:
 * - Step sequencer with velocity/swing/probability lanes
 * - Graph editor for velocity/pan/pitch/filter/mod per step
 * - Condition indicators per step
 * - Per-row mute/solo controls
 * - Per-channel params: level, pan, transpose, sample start, reverse, retrigger
 * - Pattern swing per-pattern and per-channel
 * - Fill tool with various patterns
 * - Flam/roll generator
 * - Channel colors and icons
 * - Mixer track routing
 * - Cut/cut by groups
 * - Layer channels
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
    void addChannel(const std::string& name = "New Channel", ChannelType type = ChannelType::Sampler);

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

    /**
     * @brief Set callback for channel selection
     */
    void setOnChannelSelected(std::function<void(int channel)> callback)
    {
        onChannelSelected_ = std::move(callback);
    }

    /**
     * @brief Set callback for channel double-click (opens plugin/sampler)
     */
    void setOnChannelDoubleClick(std::function<void(int channel)> callback)
    {
        onChannelDoubleClick_ = std::move(callback);
    }

private:
    std::vector<ChannelState> channels_;
    int stepsPerPattern_{16};
    int currentStep_{0};
    bool isDrawMode_{true};  // Draw mode vs select mode
    int selectedChannel_{-1};

    // View options
    bool showVelocityLane_{true};
    bool showProbabilityLane_{false};
    bool showConditionLane_{false};
    bool showMicroTimingLane_{false};
    bool showGraphEditor_{false};
    GraphTarget graphTarget_{GraphTarget::Velocity};

    // Pattern parameters
    float patternSwing_{0.0f};  ///< Pattern-level swing
    int patternLength_{16};

    // Fill tool
    FillPattern fillPattern_{FillPattern::All};
    int euclideanHits_{4};
    int euclideanSteps_{16};
    int euclideanRotation_{0};

    // Selection
    std::vector<int> selectedSteps_;
    bool isSelectingRange_{false};
    int rangeSelectStart_{-1};

    // Clipboard
    std::vector<bool> clipboardSteps_;
    std::vector<float> clipboardVelocities_;

    std::function<void(int, int, bool)> onStepChanged_;
    std::function<void(int)> onChannelSelected_;
    std::function<void(int)> onChannelDoubleClick_;

    void drawToolbar(const Theme& theme);
    void drawChannel(int index, ChannelState& channel, const Theme& theme);
    void drawChannelHeader(int index, ChannelState& channel, const Theme& theme);
    void drawStepGrid(int channelIndex, ChannelState& channel, const Theme& theme);
    void drawVelocityLane(int channelIndex, ChannelState& channel, const Theme& theme);
    void drawProbabilityLane(int channelIndex, ChannelState& channel, const Theme& theme);
    void drawConditionIndicators(int channelIndex, ChannelState& channel, const Theme& theme);
    void drawChannelParams(int channelIndex, ChannelState& channel, const Theme& theme);
    void drawGraphEditor(const Theme& theme);
    void drawFillToolPopup(const Theme& theme);
    void createDemoChannels();

    // Step operations
    void generateFlam(int channelIndex, int step, int flamCount, float flamSpacing);
    void generateRoll(int channelIndex, int startStep, int endStep, int divisions);
    void fillSteps(int channelIndex, FillPattern pattern);
    void clearSteps(int channelIndex);
    void randomizeSteps(int channelIndex, float density);
    void shiftStepsLeft(int channelIndex);
    void shiftStepsRight(int channelIndex);
    void reverseSteps(int channelIndex);
    void copySteps(int channelIndex);
    void pasteSteps(int channelIndex);

    // Euclidean rhythm generator
    std::vector<bool> generateEuclidean(int hits, int steps, int rotation);
};

} // namespace daw::ui::imgui
