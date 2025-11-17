#pragma once

#include <atomic>
#include <cstdint>

namespace daw::audio::engine
{

/**
 * @brief Transport state management
 * 
 * Thread-safe transport control for play/stop, position, tempo, and time signature.
 * Follows DAW_DEV_RULES: real-time safe, uses atomics for audio thread communication.
 * 
 * Control methods (play/stop, setTempo, etc.) are called from UI/project thread.
 * Audio thread reads state via getter methods.
 */
class Transport
{
public:
    Transport() noexcept;
    ~Transport() = default;

    // Non-copyable, non-movable (atomic state)
    Transport(const Transport&) = delete;
    Transport& operator=(const Transport&) = delete;

    // Control methods (call from UI/project thread, NOT audio thread)
    void play() noexcept;
    void stop() noexcept;
    void setPositionInBeats(double newPositionBeats) noexcept;
    void setTempo(double bpm) noexcept;
    void setTimeSignature(int numerator, int denominator) noexcept;

    // State queries (safe to call from audio thread)
    [[nodiscard]] bool isPlaying() const noexcept;
    [[nodiscard]] double getPositionInBeats() const noexcept;
    [[nodiscard]] int64_t getPositionInSamples() const noexcept;
    [[nodiscard]] double getTempo() const noexcept;
    [[nodiscard]] int getTimeSignatureNumerator() const noexcept;
    [[nodiscard]] int getTimeSignatureDenominator() const noexcept;

    // Audio thread update (called from processBlock)
    // Advances position based on samples processed and current tempo
    void updatePosition(int numSamplesProcessed, double sampleRate) noexcept;

private:
    // Thread-safe state (atomics for audio thread access)
    std::atomic<bool> playing;
    std::atomic<double> positionBeats;
    std::atomic<int64_t> positionSamples;
    std::atomic<double> tempoBpm;
    std::atomic<int> timeSigNumerator;
    std::atomic<int> timeSigDenominator;
    
    // Internal state for position calculation
    double samplesPerBeat;
    void updateSamplesPerBeat() noexcept;
};

} // namespace daw::audio::engine

