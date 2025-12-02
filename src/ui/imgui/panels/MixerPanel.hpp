#pragma once

#include "../Theme.hpp"
#include <vector>
#include <string>
#include <functional>
#include <array>

namespace daw::ui::imgui
{

/**
 * @brief Plugin slot state
 */
struct PluginSlot
{
    std::string name;
    bool bypass{false};
    bool expanded{false};
    float mix{1.0f};          // Dry/wet mix
    int pluginId{-1};         // Plugin instance ID
};

/**
 * @brief Send routing state
 */
struct SendRoute
{
    int targetChannel{-1};    // Target mixer channel
    float level{0.0f};        // Send level (0 to 1)
    bool preFader{false};     // Pre/post fader
    bool enabled{true};
};

/**
 * @brief Sidechain source
 */
struct SidechainSource
{
    int sourceChannel{-1};
    int sourceInsert{-1};     // -1 = channel output, 0+ = specific insert
    bool enabled{false};
};

/**
 * @brief Mixer channel strip state (FL Studio style)
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

    // FL Studio-style insert slots (10 slots like FL)
    std::array<PluginSlot, 10> inserts;

    // FL-style send routing
    std::vector<SendRoute> sends;

    // Sidechain
    SidechainSource sidechain;

    // Channel color (FL-style)
    ImVec4 color{0.3f, 0.5f, 0.7f, 1.0f};

    // Channel properties
    int linkedChannel{-1};    // Stereo link partner
    bool isGroup{false};      // Is this a group/bus channel
    bool isSend{false};       // Is this a send/return channel

    // Phase and width
    bool phaseInvertL{false};
    bool phaseInvertR{false};
    float stereoWidth{1.0f};  // 0 = mono, 1 = normal, 2 = widened
    float stereoOffset{0.0f}; // Stereo delay offset

    // EQ quick access
    float eqLow{0.0f};        // -12 to +12 dB
    float eqMid{0.0f};
    float eqHigh{0.0f};
    bool eqEnabled{true};
};

/**
 * @brief Mixer routing mode
 */
enum class MixerRoutingMode {
    Normal,     ///< Normal mixing
    Sidechain,  ///< Editing sidechain routes
    Sends       ///< Editing send routes
};

/**
 * @brief Mixer panel with channel strips and meters (FL Studio style)
 *
 * Features:
 * - Channel strips with faders
 * - Peak/RMS meters (animated)
 * - 10 insert slots per channel
 * - Send routing matrix
 * - Sidechain routing visualization
 * - Channel linking
 * - Phase invert and stereo width
 * - Quick EQ
 * - Master channel
 * - Channel colors
 * - Routing diagram view
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
     * @brief Add a send/return channel
     */
    void addSendChannel(const std::string& name = "Send");

    /**
     * @brief Set callback for volume change
     */
    void setOnVolumeChanged(std::function<void(int, float)> callback)
    {
        onVolumeChanged_ = std::move(callback);
    }

    /**
     * @brief Set callback for plugin slot click
     */
    void setOnPluginSlotClick(std::function<void(int channel, int slot)> callback)
    {
        onPluginSlotClick_ = std::move(callback);
    }

    /**
     * @brief Set callback for routing change
     */
    void setOnRoutingChanged(std::function<void(int from, int to, float level)> callback)
    {
        onRoutingChanged_ = std::move(callback);
    }

private:
    std::vector<MixerChannel> channels_;
    MixerChannel master_;
    int selectedChannel_{-1};
    MixerRoutingMode routingMode_{MixerRoutingMode::Normal};

    // View options
    bool showInserts_{true};
    bool showSends_{true};
    bool showEQ_{false};
    bool showRoutingDiagram_{false};
    bool compactMode_{false};
    float channelWidth_{80.0f};

    std::function<void(int, float)> onVolumeChanged_;
    std::function<void(int, int)> onPluginSlotClick_;
    std::function<void(int, int, float)> onRoutingChanged_;

    void drawToolbar(const Theme& theme);
    void drawChannelStrip(int index, MixerChannel& channel, const Theme& theme, bool isMaster = false);
    void drawMeter(const MixerChannel& channel, const Theme& theme, float width, float height);
    void drawFader(MixerChannel& channel, const Theme& theme, float width, float height);
    void drawInsertSlots(int channelIndex, MixerChannel& channel, const Theme& theme);
    void drawSendKnobs(int channelIndex, MixerChannel& channel, const Theme& theme);
    void drawQuickEQ(MixerChannel& channel, const Theme& theme);
    void drawStereoControls(MixerChannel& channel, const Theme& theme);
    void drawRoutingDiagram(const Theme& theme);
    void drawSidechainIndicator(int channelIndex, const MixerChannel& channel, const Theme& theme);
    void updateMeters();
    void createDemoChannels();
};

} // namespace daw::ui::imgui
