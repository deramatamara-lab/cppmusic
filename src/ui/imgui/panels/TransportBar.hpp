#pragma once

#include "../Theme.hpp"

namespace daw::ui::imgui
{

/**
 * @brief Transport state for playback control
 */
struct TransportState
{
    bool isPlaying{false};
    bool isRecording{false};
    bool isLooping{false};
    bool metronomeEnabled{false};
    
    double bpm{120.0};
    int beatsPerBar{4};
    int beatUnit{4};
    
    double positionBeats{0.0};
    double loopStartBeats{0.0};
    double loopEndBeats{16.0};
    
    float cpuUsage{0.0f};
};

/**
 * @brief Transport bar panel with playback controls
 * 
 * Features:
 * - Play/Stop/Record buttons
 * - BPM control
 * - Time signature
 * - Position display
 * - Metronome toggle
 * - CPU meter
 */
class TransportBar
{
public:
    TransportBar();
    ~TransportBar() = default;

    /**
     * @brief Draw the transport bar
     * @param theme Theme for styling
     */
    void draw(const Theme& theme);

    /**
     * @brief Get transport state
     */
    [[nodiscard]] TransportState& getState() { return state_; }
    [[nodiscard]] const TransportState& getState() const { return state_; }

    /**
     * @brief Set callback for play/pause
     */
    void setOnPlay(std::function<void(bool)> callback) { onPlay_ = std::move(callback); }
    
    /**
     * @brief Set callback for stop
     */
    void setOnStop(std::function<void()> callback) { onStop_ = std::move(callback); }
    
    /**
     * @brief Set callback for record
     */
    void setOnRecord(std::function<void(bool)> callback) { onRecord_ = std::move(callback); }
    
    /**
     * @brief Set callback for BPM change
     */
    void setOnBpmChange(std::function<void(double)> callback) { onBpmChange_ = std::move(callback); }

private:
    TransportState state_;
    
    std::function<void(bool)> onPlay_;
    std::function<void()> onStop_;
    std::function<void(bool)> onRecord_;
    std::function<void(double)> onBpmChange_;

    void drawPlayButton(const Theme& theme);
    void drawStopButton(const Theme& theme);
    void drawRecordButton(const Theme& theme);
    void drawBpmControl(const Theme& theme);
    void drawTimeSignature(const Theme& theme);
    void drawPositionDisplay(const Theme& theme);
    void drawMetronome(const Theme& theme);
    void drawCpuMeter(const Theme& theme);
    
    static std::string formatTime(double beats, double bpm, int beatsPerBar);
    static std::string formatPosition(double beats, int beatsPerBar);
};

} // namespace daw::ui::imgui
