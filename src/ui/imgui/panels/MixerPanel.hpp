#pragma once

#include "../Theme.hpp"
#include <vector>
#include <string>
#include <functional>

namespace daw::ui::imgui
{

/**
 * @brief Mixer channel strip state
 */
struct MixerChannel
{
    std::string name{"Channel"};
    float volume{0.8f};         // 0.0 - 1.0
    float pan{0.5f};            // 0.0 (L) - 1.0 (R)
    float peakL{0.0f};          // Current peak level L
    float peakR{0.0f};          // Current peak level R
    float rmsL{0.0f};           // RMS level L
    float rmsR{0.0f};           // RMS level R
    bool muted{false};
    bool soloed{false};
    bool armed{false};          // Record armed
    std::vector<std::string> inserts;  // Insert effect names
    std::vector<std::string> sends;    // Send names
};

/**
 * @brief Mixer panel with channel strips and meters
 * 
 * Features:
 * - Channel strips with faders
 * - Peak/RMS meters (animated)
 * - Insert/send placeholders
 * - Master channel
 */
class MixerPanel
{
public:
    MixerPanel();
    ~MixerPanel() = default;

    /**
     * @brief Draw the mixer panel
     * @param open Reference to visibility flag
     * @param theme Theme for styling
     */
    void draw(bool& open, const Theme& theme);

    /**
     * @brief Get channels
     */
    [[nodiscard]] std::vector<MixerChannel>& getChannels() { return channels_; }

    /**
     * @brief Get master channel
     */
    [[nodiscard]] MixerChannel& getMaster() { return master_; }

    /**
     * @brief Add a channel
     */
    void addChannel(const std::string& name = "Channel");

    /**
     * @brief Set callback for volume change
     */
    void setOnVolumeChanged(std::function<void(int, float)> callback)
    {
        onVolumeChanged_ = std::move(callback);
    }

private:
    std::vector<MixerChannel> channels_;
    MixerChannel master_;
    int selectedChannel_{-1};
    
    std::function<void(int, float)> onVolumeChanged_;

    void drawChannelStrip(int index, MixerChannel& channel, const Theme& theme, bool isMaster = false);
    void drawMeter(const MixerChannel& channel, const Theme& theme, float width, float height);
    void drawFader(MixerChannel& channel, const Theme& theme, float width, float height);
    void updateMeters();
    void createDemoChannels();
};

} // namespace daw::ui::imgui
