#pragma once

#include "../Theme.hpp"
#include <functional>

namespace daw::ui::imgui
{

/**
 * @brief Transport mode (FL Studio style)
 */
enum class TransportMode {
    Song,       ///< Play entire arrangement
    Pattern     ///< Play current pattern only
};

/**
 * @brief Recording mode
 */
enum class RecordMode {
    Notes,      ///< Record MIDI notes
    Automation, ///< Record automation
    Audio,      ///< Record audio
    NotesPunch, ///< Punch-in note recording
    Score       ///< Step recording (FL-style)
};

/**
 * @brief Count-in mode
 */
enum class CountInMode {
    Off,
    OneBar,
    TwoBars,
    FourBars
};

/**
 * @brief Transport state for playback control
 */
struct TransportState
{
    bool isPlaying{false};
    bool isRecording{false};
    bool isLooping{false};
    bool metronomeEnabled{false};

    // FL Studio-style mode
    TransportMode mode{TransportMode::Song};
    RecordMode recordMode{RecordMode::Notes};
    CountInMode countIn{CountInMode::Off};

    double bpm{120.0};
    int beatsPerBar{4};
    int beatUnit{4};

    double positionBeats{0.0};
    double loopStartBeats{0.0};
    double loopEndBeats{16.0};

    // FL-style pattern info
    int currentPattern{1};
    int totalPatterns{1};
    std::string patternName{"Pattern 1"};

    // Metronome settings
    bool metronomeOnlyInRecord{false};
    int metronomePreCount{0};
    float metronomeVolume{0.8f};

    // Tap tempo
    double lastTapTime{0.0};
    std::vector<double> tapHistory;

    float cpuUsage{0.0f};
    float diskUsage{0.0f};   // Disk streaming usage
    int voiceCount{0};       // Active voice count
    int polyLimit{256};      // Polyphony limit
};

/**
 * @brief Transport bar panel with playback controls
 *
 * Features (FL Studio style):
 * - Play/Stop/Record buttons
 * - Song/Pattern mode toggle
 * - BPM control with tap tempo
 * - Time signature
 * - Position display (bars:beats:ticks)
 * - Metronome toggle with settings
 * - Count-in options
 * - CPU/Disk/Voice meters
 * - Pattern selector
 * - Recording mode selector
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

    /**
     * @brief Set callback for pattern change
     */
    void setOnPatternChange(std::function<void(int)> callback) { onPatternChange_ = std::move(callback); }

    /**
     * @brief Set callback for mode change
     */
    void setOnModeChange(std::function<void(TransportMode)> callback) { onModeChange_ = std::move(callback); }

    /**
     * @brief Handle tap tempo input
     */
    void tapTempo();

private:
    TransportState state_;

    std::function<void(bool)> onPlay_;
    std::function<void()> onStop_;
    std::function<void(bool)> onRecord_;
    std::function<void(double)> onBpmChange_;
    std::function<void(int)> onPatternChange_;
    std::function<void(TransportMode)> onModeChange_;

    // Internal state
    bool showMetronomeSettings_{false};
    bool showRecordSettings_{false};
    double lastFrameTime_{0.0};

    void drawPlayButton(const Theme& theme);
    void drawStopButton(const Theme& theme);
    void drawRecordButton(const Theme& theme);
    void drawModeToggle(const Theme& theme);
    void drawPatternSelector(const Theme& theme);
    void drawBpmControl(const Theme& theme);
    void drawTapTempoButton(const Theme& theme);
    void drawTimeSignature(const Theme& theme);
    void drawPositionDisplay(const Theme& theme);
    void drawMetronome(const Theme& theme);
    void drawMetronomeSettings(const Theme& theme);
    void drawCountIn(const Theme& theme);
    void drawRecordModeSelector(const Theme& theme);
    void drawCpuMeter(const Theme& theme);
    void drawVoiceMeter(const Theme& theme);
    void drawHintDisplay(const Theme& theme);

    static std::string formatTime(double beats, double bpm, int beatsPerBar);
    static std::string formatPosition(double beats, int beatsPerBar);
    static std::string formatPositionTicks(double beats, int beatsPerBar, int ppq);
};

} // namespace daw::ui::imgui
