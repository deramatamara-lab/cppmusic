#pragma once
/**
 * @file Transport.hpp
 * @brief Sample-accurate beat position advancement and tempo control.
 *
 * This is part of the foundational engine skeleton for the cppmusic DAW.
 * Provides thread-safe transport state management for playback control.
 */

#include <atomic>
#include <cstdint>

namespace cppmusic::engine {

/**
 * @brief Transport state management for audio playback.
 *
 * Thread-safe transport control for play/stop, position, tempo, and time signature.
 * Uses atomics for safe audio thread communication.
 *
 * Control methods (play/stop, setTempo, etc.) are called from the UI/project thread.
 * Audio thread reads state via getter methods and calls advancePosition().
 *
 * Real-time safety:
 * - All getters are noexcept and lock-free.
 * - advancePosition() is noexcept, lock-free, and allocation-free.
 */
class Transport {
public:
    /**
     * @brief Playback state enumeration.
     */
    enum class State : std::uint8_t {
        Stopped = 0,
        Playing = 1,
        Paused = 2
    };

    Transport() noexcept;
    ~Transport() = default;

    // Non-copyable, non-movable (atomic state)
    Transport(const Transport&) = delete;
    Transport& operator=(const Transport&) = delete;
    Transport(Transport&&) = delete;
    Transport& operator=(Transport&&) = delete;

    // =========================================================================
    // Control Methods (call from UI/project thread, NOT audio thread)
    // =========================================================================

    /**
     * @brief Start playback.
     */
    void play() noexcept;

    /**
     * @brief Stop playback and reset position to zero.
     */
    void stop() noexcept;

    /**
     * @brief Pause playback without resetting position.
     */
    void pause() noexcept;

    /**
     * @brief Set the playback position in beats.
     * @param beats The new position in beats.
     */
    void setPositionBeats(double beats) noexcept;

    /**
     * @brief Set the playback position in samples.
     * @param samples The new position in samples.
     */
    void setPositionSamples(std::int64_t samples) noexcept;

    /**
     * @brief Set the tempo in beats per minute.
     * @param bpm Tempo value (clamped to [20.0, 999.0]).
     */
    void setTempo(double bpm) noexcept;

    /**
     * @brief Set the time signature.
     * @param numerator Beats per measure (clamped to [1, 32]).
     * @param denominator Beat unit (clamped to [1, 32]).
     */
    void setTimeSignature(int numerator, int denominator) noexcept;

    /**
     * @brief Set the sample rate for timing calculations.
     * @param rate Sample rate in Hz.
     */
    void setSampleRate(double rate) noexcept;

    // =========================================================================
    // State Queries (safe to call from audio thread)
    // =========================================================================

    /**
     * @brief Get the current playback state.
     */
    [[nodiscard]] State getState() const noexcept;

    /**
     * @brief Check if transport is currently playing.
     */
    [[nodiscard]] bool isPlaying() const noexcept;

    /**
     * @brief Get the current playback position in beats.
     */
    [[nodiscard]] double getPositionBeats() const noexcept;

    /**
     * @brief Get the current playback position in samples.
     */
    [[nodiscard]] std::int64_t getPositionSamples() const noexcept;

    /**
     * @brief Get the current tempo in BPM.
     */
    [[nodiscard]] double getTempo() const noexcept;

    /**
     * @brief Get the time signature numerator (beats per measure).
     */
    [[nodiscard]] int getTimeSignatureNumerator() const noexcept;

    /**
     * @brief Get the time signature denominator (beat unit).
     */
    [[nodiscard]] int getTimeSignatureDenominator() const noexcept;

    /**
     * @brief Get the sample rate.
     */
    [[nodiscard]] double getSampleRate() const noexcept;

    // =========================================================================
    // Audio Thread Methods
    // =========================================================================

    /**
     * @brief Advance the playback position by a number of samples.
     * @param numSamples Number of samples processed.
     *
     * Called from the audio thread during processBlock().
     * Real-time safe: noexcept, lock-free, allocation-free.
     */
    void advancePosition(int numSamples) noexcept;

    // =========================================================================
    // Utility Methods
    // =========================================================================

    /**
     * @brief Convert beats to samples at current tempo and sample rate.
     * @param beats Number of beats.
     * @return Corresponding number of samples.
     */
    [[nodiscard]] std::int64_t beatsToSamples(double beats) const noexcept;

    /**
     * @brief Convert samples to beats at current tempo and sample rate.
     * @param samples Number of samples.
     * @return Corresponding number of beats.
     */
    [[nodiscard]] double samplesToBeats(std::int64_t samples) const noexcept;

private:
    // Thread-safe state (atomics for audio thread access)
    std::atomic<State> state_{State::Stopped};
    std::atomic<double> positionBeats_{0.0};
    std::atomic<std::int64_t> positionSamples_{0};
    std::atomic<double> tempoBpm_{120.0};
    std::atomic<int> timeSigNumerator_{4};
    std::atomic<int> timeSigDenominator_{4};
    std::atomic<double> sampleRate_{44100.0};

    /**
     * @brief Calculate samples per beat based on current tempo and sample rate.
     */
    [[nodiscard]] double getSamplesPerBeat() const noexcept;
};

} // namespace cppmusic::engine
