#pragma once

/**
 * @file PatternPlaybackEngine.h
 * @brief High-level pattern playback orchestrator for deterministic MIDI rendering.
 */

#include "PatternPlayer.h"
#include "../../core/utilities/Logger.h"

#include <atomic>

namespace daw::audio::engine
{

/**
 * @brief Thread-safe coordinator around PatternPlayer.
 *
 * Bridges transport state and project patterns to MIDI generation while
 * respecting DAW_DEV_RULES (no allocations on the audio thread, deterministic
 * behaviour, explicit logging).
 */
class PatternPlaybackEngine
{
public:
    PatternPlaybackEngine() noexcept;
    ~PatternPlaybackEngine() = default;

    PatternPlaybackEngine(const PatternPlaybackEngine&) = delete;
    PatternPlaybackEngine& operator=(const PatternPlaybackEngine&) = delete;
    PatternPlaybackEngine(PatternPlaybackEngine&&) noexcept = delete;
    PatternPlaybackEngine& operator=(PatternPlaybackEngine&&) noexcept = delete;

    /**
     * @brief Prepare the engine for audio processing.
     * @param sampleRate active sample rate in Hz.
     * @param maxBlockSize maximum block size in samples.
     */
    void prepare(double sampleRate, int maxBlockSize) noexcept;

    /**
     * @brief Reset internal state, clearing transport caches.
     */
    void reset() noexcept;

    /**
     * @brief Assign an active pattern for playback.
     * @param pattern pointer owned by ProjectModel (must remain valid).
     */
    void setPattern(const daw::project::Pattern* pattern) noexcept;

    /**
     * @brief Clear the active pattern (disables playback).
     */
    void clearPattern() noexcept;

    /**
     * @brief Adjust quantisation grid.
     * @param gridDivision division in beats (e.g. 1/16).
     */
    void setQuantization(double gridDivision) noexcept;

    /**
     * @brief Render pattern events into a MIDI buffer.
     *
     * @param buffer target MIDI buffer (append-only).
     * @param numSamples block size.
     * @param startBeat transport beat at block start.
     * @param tempoBpm transport tempo.
     */
    void processBlock(juce::MidiBuffer& buffer, int numSamples, double startBeat, double tempoBpm) noexcept;

    /**
     * @brief Returns whether a pattern is currently active.
     */
    [[nodiscard]] bool hasActivePattern() const noexcept;

private:
    PatternPlayer patternPlayer;
    std::atomic<const daw::project::Pattern*> pendingPattern{nullptr};
    std::atomic<const daw::project::Pattern*> activePattern{nullptr};
    std::atomic<double> quantizationDivision{1.0 / 16.0};
    std::atomic<bool> patternDirty{false};

    void syncPatternIfNeeded() noexcept;
};

} // namespace daw::audio::engine
