#pragma once

#include "../processors/AudioProcessorBase.h"
#include "Oscillator.h"
#include "../dsp/Envelope.h"
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>
#include <vector>
#include <memory>

namespace daw::audio::synthesis
{

/**
 * @brief Professional subtractive synthesizer
 *
 * Multi-oscillator polyphonic synthesizer with ADSR envelopes and low-pass filter.
 * Real-time safe, lock-free parameter updates, pre-allocated voice pool.
 *
 * Features:
 * - Up to 3 oscillators per voice
 * - Waveform types: sine, square, sawtooth, triangle, noise
 * - ADSR envelope per voice
 * - Moog-style ladder filter
 * - Polyphonic (up to 16 voices)
 * - MIDI note on/off handling
 * - Velocity sensitivity
 *
 * Follows DAW_DEV_RULES: no allocations in processBlock, thread-safe parameters.
 */
class Synthesizer : public processors::AudioProcessorBase
{
public:
    // Use Oscillator's Waveform enum for compatibility
    using Waveform = Oscillator::Waveform;

    Synthesizer();
    ~Synthesizer() override = default;

    void prepareToPlay(double sampleRate, int maximumBlockSize) override;
    void processBlock(float* buffer, int numSamples) noexcept override;
    void reset() override;

    /**
     * @brief Process MIDI messages
     * @param midiBuffer MIDI buffer to process
     */
    void processMidi(const juce::MidiBuffer& midiBuffer) noexcept;

    /**
     * @brief Set filter cutoff frequency in Hz
     * @param cutoffHz Cutoff frequency (20.0 to 20000.0 Hz)
     */
    void setFilterCutoff(float cutoffHz) noexcept;

    /**
     * @brief Get current filter cutoff
     * @return Cutoff frequency in Hz
     */
    [[nodiscard]] float getFilterCutoff() const noexcept;

    /**
     * @brief Set filter resonance (0.0 to 1.0)
     * @param resonance Resonance amount (0.0 = no resonance, 1.0 = maximum)
     */
    void setFilterResonance(float resonance) noexcept;

    /**
     * @brief Get current filter resonance
     * @return Resonance amount (0.0 to 1.0)
     */
    [[nodiscard]] float getFilterResonance() const noexcept;

    /**
     * @brief Set oscillator waveform
     * @param oscIndex Oscillator index (0-2)
     * @param waveform Waveform type
     */
    void setOscillatorWaveform(int oscIndex, Waveform waveform) noexcept;

    /**
     * @brief Set oscillator level
     * @param oscIndex Oscillator index (0-2)
     * @param level Level (0.0 to 1.0)
     */
    void setOscillatorLevel(int oscIndex, float level) noexcept;

    /**
     * @brief Set attack time in seconds
     */
    void setAttackTime(float attackSeconds) noexcept;

    /**
     * @brief Set decay time in seconds
     */
    void setDecayTime(float decaySeconds) noexcept;

    /**
     * @brief Set sustain level (0.0 to 1.0)
     */
    void setSustainLevel(float sustainLevel) noexcept;

    /**
     * @brief Set release time in seconds
     */
    void setReleaseTime(float releaseSeconds) noexcept;

private:
    static constexpr int MAX_VOICES = 16;
    static constexpr int NUM_OSCILLATORS = 3;
    static constexpr float DENORMAL_PREVENTION = 1e-20f;
    static constexpr float MIDI_NOTE_A4 = 69.0f;
    static constexpr float A4_FREQUENCY = 440.0f;

    // Voice structure
    struct Voice
    {
        std::array<Oscillator, NUM_OSCILLATORS> oscillators;
        std::unique_ptr<dsp::Envelope> envelope;
        float filterState[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
        int noteNumber{-1};
        float velocity{0.0f};
        bool isActive{false};

        Voice();
    };

    // Thread-safe parameters
    std::atomic<float> filterCutoff{1000.0f};
    std::atomic<float> filterResonance{0.5f};
    std::atomic<Waveform> oscWaveforms[NUM_OSCILLATORS];
    std::atomic<float> oscLevels[NUM_OSCILLATORS];
    std::atomic<float> attackTime{0.01f};
    std::atomic<float> decayTime{0.1f};
    std::atomic<float> sustainLevel{0.7f};
    std::atomic<float> releaseTime{0.2f};

    // Voice pool (pre-allocated for real-time safety)
    std::array<Voice, MAX_VOICES> voices;

    // Pre-allocated envelope buffer (for real-time safety)
    std::vector<float> envelopeBuffer;

    // Filter coefficients (updated from parameters)
    float filterCoeff{0.0f};
    float filterRes{0.0f};

    // Helper functions
    [[nodiscard]] static float midiNoteToFrequency(float noteNumber) noexcept;
    [[nodiscard]] Voice* findFreeVoice() noexcept;
    [[nodiscard]] Voice* findVoiceForNote(int noteNumber) noexcept;
    void processVoice(Voice& voice, float* output, int numSamples) noexcept;
    float processFilter(float input, Voice& voice) noexcept;
    void updateFilterCoefficients() noexcept;
};

} // namespace daw::audio::synthesis
