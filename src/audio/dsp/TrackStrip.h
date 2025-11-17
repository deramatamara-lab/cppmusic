#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cstdint>

namespace daw::audio::dsp
{

/**
 * @brief Per-track audio processor
 * 
 * Applies gain, pan, mute, and solo to a track.
 * Provides lock-free metering (peak/RMS).
 * Follows DAW_DEV_RULES: real-time safe, no allocations/locks in processBlock.
 */
class TrackStrip
{
public:
    TrackStrip() noexcept;
    ~TrackStrip() = default;

    // Non-copyable, non-movable (atomics prevent safe moves)
    TrackStrip(const TrackStrip&) = delete;
    TrackStrip& operator=(const TrackStrip&) = delete;

    // Setup (call from non-audio thread)
    void prepareToPlay(double sampleRate, int maximumBlockSize) noexcept;
    void releaseResources() noexcept;
    void reset() noexcept;

    // Parameter control (call from UI/project thread, NOT audio thread)
    void setGain(float gainDb) noexcept;
    void setPan(float pan) noexcept; // -1.0 (left) to 1.0 (right)
    void setMute(bool muted) noexcept;
    void setSolo(bool soloed) noexcept;

    // Parameter queries (safe from any thread)
    [[nodiscard]] float getGain() const noexcept;
    [[nodiscard]] float getPan() const noexcept;
    [[nodiscard]] bool isMuted() const noexcept;
    [[nodiscard]] bool isSoloed() const noexcept;

    // Audio processing (real-time safe, called from audio thread)
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) noexcept;

    // Metering (lock-free, safe to call from UI thread)
    [[nodiscard]] float getPeakLevel() const noexcept;
    [[nodiscard]] float getRmsLevel() const noexcept;
    void resetMeters() noexcept;

private:
    // Thread-safe parameters (atomics for audio thread access)
    std::atomic<float> gainLinear;
    std::atomic<float> pan;
    std::atomic<bool> muted;
    std::atomic<bool> soloed;
    
    // Metering (lock-free, updated in audio thread, read from UI thread)
    std::atomic<float> peakLevel;
    std::atomic<float> rmsLevel;
    
    // Internal state
    double currentSampleRate;
    int currentBlockSize;
    
    // Helper functions
    [[nodiscard]] static float dbToLinear(float db) noexcept;
    void updateMeters(const juce::AudioBuffer<float>& buffer) noexcept;
};

} // namespace daw::audio::dsp

