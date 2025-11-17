#pragma once

#include "../../project/Pattern.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <atomic>
#include <cstdint>

namespace daw::audio::engine
{

/**
 * @brief Pattern execution engine
 * 
 * Plays pattern clips with quantization and variations.
 * Follows DAW_DEV_RULES: real-time safe, no allocations in processBlock.
 */
class PatternPlayer
{
public:
    PatternPlayer() noexcept;
    ~PatternPlayer() = default;

    // Non-copyable, movable
    PatternPlayer(const PatternPlayer&) = delete;
    PatternPlayer& operator=(const PatternPlayer&) = delete;
    PatternPlayer(PatternPlayer&&) noexcept = default;
    PatternPlayer& operator=(PatternPlayer&&) noexcept = default;

    /**
     * @brief Prepare pattern player for playback
     * @param sampleRate Current sample rate
     * @param maxBlockSize Maximum block size
     */
    void prepareToPlay(double sampleRate, int maxBlockSize) noexcept;

    /**
     * @brief Release resources
     */
    void releaseResources() noexcept;

    /**
     * @brief Reset pattern player
     */
    void reset() noexcept;

    /**
     * @brief Set pattern to play
     * @param pattern Pattern to play (must remain valid during playback)
     */
    void setPattern(const daw::project::Pattern* pattern) noexcept;

    /**
     * @brief Set quantization (1/4, 1/8, 1/16, 1/32, etc.)
     */
    void setQuantization(double gridDivision) noexcept;

    /**
     * @brief Process pattern playback for a block
     * @param buffer MIDI buffer to write notes to
     * @param numSamples Number of samples
     * @param currentBeat Current beat position
     * @param tempoBpm Current tempo in BPM
     */
    void processBlock(juce::MidiBuffer& buffer, int numSamples, double currentBeat, double tempoBpm) noexcept;

    /**
     * @brief Check if pattern is playing
     */
    [[nodiscard]] bool isPlaying() const noexcept { return pattern != nullptr; }

private:
    const daw::project::Pattern* pattern{nullptr};
    double quantization{1.0 / 16.0}; // 1/16 note quantization by default
    double currentSampleRate{44100.0};
    int currentBlockSize{128};
    
    double lastBeatPosition{0.0};
    std::vector<daw::project::Pattern::MIDINote> pendingNotes;
    
    void scheduleNotes(double startBeat, double endBeat, double tempoBpm, int blockSamples, juce::MidiBuffer& buffer) noexcept;
    [[nodiscard]] double beatsToSamples(double beats, double tempoBpm) const noexcept;
};

} // namespace daw::audio::engine

