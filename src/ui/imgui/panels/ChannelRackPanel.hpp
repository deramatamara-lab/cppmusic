#pragma once

#include "../Theme.hpp"
#include <string>
#include <vector>
#include <functional>

namespace daw::ui::imgui
{

/**
 * @brief Channel state for channel rack
 */
struct ChannelState
{
    std::string name{"Channel"};
    bool muted{false};
    bool soloed{false};
    float volume{0.8f};
    float pan{0.5f};
    std::vector<bool> steps;  // Pattern steps (16 by default)
    std::vector<float> velocities;  // Velocity per step
};

/**
 * @brief Channel Rack panel for pattern-based sequencing
 * 
 * Features:
 * - Pattern step grid with draw/select modes
 * - Per-row mute/solo controls
 * - Velocity lane
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

    std::function<void(int, int, bool)> onStepChanged_;

    void drawChannel(int index, ChannelState& channel, const Theme& theme);
    void drawStepGrid(int channelIndex, ChannelState& channel, const Theme& theme);
    void drawVelocityLane(int channelIndex, ChannelState& channel, const Theme& theme);
    void createDemoChannels();
};

} // namespace daw::ui::imgui
