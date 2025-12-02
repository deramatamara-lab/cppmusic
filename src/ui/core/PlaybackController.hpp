#pragma once
/**
 * @file PlaybackController.hpp
 * @brief Controller bridging UI actions to the real-time engine.
 *
 * The PlaybackController is a dedicated controller layer that keeps the audio
 * thread isolated from JUCE UI. It provides thread-safe methods callable from
 * the UI thread and maintains a lock-free shared state snapshot for UI updates.
 */

#include "../../audio/engine/EngineContext.h"
#include "../../project/ProjectModel.h"
#include <atomic>
#include <memory>
#include <functional>

namespace daw::ui::core
{

/**
 * @brief Read-only snapshot of playback state for UI display.
 *
 * This structure contains all the transport/playback state that the UI
 * needs to display. It is updated atomically from the controller and
 * can be safely read from the UI thread.
 */
struct PlaybackState
{
    double positionBeats{0.0};      ///< Current playhead position in beats
    int64_t positionSamples{0};     ///< Current playhead position in samples
    double tempo{120.0};            ///< Current tempo in BPM
    int timeSignatureNumerator{4};  ///< Time signature numerator
    int timeSignatureDenominator{4};///< Time signature denominator
    bool playing{false};            ///< Whether transport is playing
    bool loopEnabled{false};        ///< Whether loop is enabled
    double loopStartBeats{0.0};     ///< Loop start position in beats
    double loopEndBeats{4.0};       ///< Loop end position in beats
    bool metronomeEnabled{false};   ///< Whether metronome is enabled
    float cpuLoad{0.0f};            ///< Current CPU load (0.0-1.0)
    float masterPeak{0.0f};         ///< Master output peak level
    float masterRms{0.0f};          ///< Master output RMS level
};

/**
 * @brief Listener interface for playback state changes.
 */
class PlaybackListener
{
public:
    virtual ~PlaybackListener() = default;
    
    /// Called when transport state changes (play/stop)
    virtual void onTransportStateChanged(bool playing) { juce::ignoreUnused(playing); }
    
    /// Called when position changes (during playback or seek)
    virtual void onPositionChanged(double positionBeats) { juce::ignoreUnused(positionBeats); }
    
    /// Called when tempo changes
    virtual void onTempoChanged(double bpm) { juce::ignoreUnused(bpm); }
    
    /// Called when loop region changes
    virtual void onLoopChanged(bool enabled, double startBeats, double endBeats) 
    { 
        juce::ignoreUnused(enabled, startBeats, endBeats); 
    }
};

/**
 * @brief Controller bridging UI actions to the real-time engine.
 *
 * This controller owns references to:
 * - EngineContext (transport, audio graph)
 * - ProjectModel (patterns, tracks, clips)
 *
 * It provides thread-safe methods callable from the UI:
 * - play(), stop(), toggleLoop()
 * - setTempo(), seekToBeats()
 * - setLoopRegion()
 *
 * It maintains an internal, lock-free shared state snapshot that can
 * be periodically queried by the UI via getCurrentState().
 */
class PlaybackController
{
public:
    /**
     * @brief Construct a PlaybackController.
     * @param engineContext Shared pointer to the engine context.
     * @param projectModel Shared pointer to the project model.
     */
    PlaybackController(
        std::shared_ptr<daw::audio::engine::EngineContext> engineContext,
        std::shared_ptr<daw::project::ProjectModel> projectModel);

    ~PlaybackController();

    // Non-copyable, non-movable
    PlaybackController(const PlaybackController&) = delete;
    PlaybackController& operator=(const PlaybackController&) = delete;
    PlaybackController(PlaybackController&&) = delete;
    PlaybackController& operator=(PlaybackController&&) = delete;

    // =========================================================================
    // Transport Control (call from UI thread)
    // =========================================================================

    /**
     * @brief Start playback.
     */
    void play();

    /**
     * @brief Stop playback and optionally reset position.
     * @param resetPosition If true, resets position to 0.
     */
    void stop(bool resetPosition = false);

    /**
     * @brief Toggle between play and stop.
     */
    void togglePlayStop();

    /**
     * @brief Toggle loop mode on/off.
     */
    void toggleLoop();

    /**
     * @brief Seek to a specific beat position.
     * @param beats The target position in beats.
     */
    void seekToBeats(double beats);

    /**
     * @brief Set the playback tempo.
     * @param bpm Tempo in beats per minute.
     */
    void setTempo(double bpm);

    /**
     * @brief Set the time signature.
     * @param numerator Beats per measure.
     * @param denominator Beat unit.
     */
    void setTimeSignature(int numerator, int denominator);

    /**
     * @brief Set the loop region.
     * @param startBeats Loop start position in beats.
     * @param endBeats Loop end position in beats.
     */
    void setLoopRegion(double startBeats, double endBeats);

    /**
     * @brief Enable or disable the loop.
     * @param enabled Whether loop is enabled.
     */
    void setLoopEnabled(bool enabled);

    /**
     * @brief Enable or disable the metronome.
     * @param enabled Whether metronome is enabled.
     */
    void setMetronomeEnabled(bool enabled);

    /**
     * @brief Set metronome volume.
     * @param volume Volume level (0.0-1.0).
     */
    void setMetronomeVolume(float volume);

    // =========================================================================
    // State Queries (safe from UI thread)
    // =========================================================================

    /**
     * @brief Get the current playback state snapshot.
     * @return A read-only snapshot of the current playback state.
     *
     * This method is safe to call from the UI thread. The returned state
     * is a consistent snapshot and will not change during the UI's use.
     */
    [[nodiscard]] PlaybackState getCurrentState() const;

    /**
     * @brief Check if transport is currently playing.
     */
    [[nodiscard]] bool isPlaying() const;

    /**
     * @brief Get the current position in beats.
     */
    [[nodiscard]] double getPositionBeats() const;

    /**
     * @brief Get the current tempo.
     */
    [[nodiscard]] double getTempo() const;

    /**
     * @brief Check if loop is enabled.
     */
    [[nodiscard]] bool isLoopEnabled() const;

    // =========================================================================
    // Listener Management
    // =========================================================================

    /**
     * @brief Add a listener for playback state changes.
     * @param listener The listener to add.
     */
    void addListener(PlaybackListener* listener);

    /**
     * @brief Remove a listener.
     * @param listener The listener to remove.
     */
    void removeListener(PlaybackListener* listener);

    // =========================================================================
    // Update Method (call from UI timer)
    // =========================================================================

    /**
     * @brief Update the internal state snapshot from the engine.
     *
     * This method should be called periodically (e.g., 30-60 Hz) from
     * a timer on the UI thread to keep the state snapshot current.
     */
    void updateStateFromEngine();

    // =========================================================================
    // Access to underlying components
    // =========================================================================

    /**
     * @brief Get the engine context.
     */
    [[nodiscard]] std::shared_ptr<daw::audio::engine::EngineContext> getEngineContext() const 
    { 
        return engineContext_; 
    }

    /**
     * @brief Get the project model.
     */
    [[nodiscard]] std::shared_ptr<daw::project::ProjectModel> getProjectModel() const 
    { 
        return projectModel_; 
    }

private:
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext_;
    std::shared_ptr<daw::project::ProjectModel> projectModel_;

    // Lock-free state for UI reads
    mutable std::atomic<double> positionBeats_{0.0};
    mutable std::atomic<bool> playing_{false};
    mutable std::atomic<double> tempo_{120.0};
    mutable std::atomic<bool> loopEnabled_{false};
    mutable std::atomic<double> loopStartBeats_{0.0};
    mutable std::atomic<double> loopEndBeats_{4.0};
    mutable std::atomic<bool> metronomeEnabled_{false};

    // Listeners
    std::vector<PlaybackListener*> listeners_;
    
    // Helper to notify listeners
    void notifyTransportStateChanged(bool playing);
    void notifyPositionChanged(double positionBeats);
    void notifyTempoChanged(double bpm);
    void notifyLoopChanged(bool enabled, double startBeats, double endBeats);
};

} // namespace daw::ui::core
