#pragma once
/**
 * @file AudioEngine.hpp
 * @brief Real-time audio engine using SDL2 for actual sound output
 *
 * This provides actual audio playback - not a stub!
 */

#include <SDL.h>
#include <atomic>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include <functional>
#include <mutex>
#include <string>

namespace daw::ui::imgui
{
// Simple synthesis waveform types
enum class Waveform { Sine, Square, Saw, Triangle, Noise };

/**
 * @brief A single synthesizer voice
 */
struct SynthVoice
{
    bool active{false};
    int note{60};               // MIDI note (60 = C4)
    float velocity{0.8f};       // 0-1
    double phase{0.0};          // Oscillator phase
    float envelope{0.0f};       // Current envelope value
    float envTarget{0.0f};      // Envelope target (attack/release)
    float envRate{0.0f};        // Envelope rate
    Waveform waveform{Waveform::Saw};
    uint32_t rng{0x12345678};   // For noise

    // Pitch envelope for kick drums
    float pitchEnv{0.0f};       // Pitch envelope value
    float pitchEnvDecay{0.0f};  // Pitch decay rate
    int baseNote{60};           // Base note before pitch env

    // Sample counter for decay
    int sampleCount{0};
};

/**
 * @brief A single step in a pattern
 */
struct PatternStep
{
    bool active{false};
    int note{60};               // MIDI note
    float velocity{0.8f};       // Velocity 0-1
    float pan{0.0f};            // Pan -1 to 1
};

/**
 * @brief A channel in the channel rack
 */
struct Channel
{
    std::string name = "Channel";
    Waveform waveform = Waveform::Saw;
    float volume = 0.8f;
    float pan = 0.0f;
    bool muted = false;
    bool soloed = false;

    // 16 steps per pattern
    std::array<PatternStep, 16> steps = {};

    // Active voice for this channel
    SynthVoice voice;
};

/**
 * @brief A pattern containing multiple channels
 */
struct Pattern
{
    std::string name = "Pattern 1";
    int length = 16;             // Steps per pattern
    std::vector<Channel> channels;
};
/**
 * @brief Real-time audio engine with step sequencer
 */
class AudioEngine
{
public:
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int BUFFER_SIZE = 512;
    static constexpr int NUM_CHANNELS = 2;

    AudioEngine();
    ~AudioEngine();

    // Lifecycle
    bool initialize();
    void shutdown();

    // Transport
    void play();
    void stop();
    void pause();
    bool isPlaying() const { return isPlaying_.load(); }

    // Tempo & Position
    void setBPM(double bpm);
    double getBPM() const { return bpm_.load(); }
    int getCurrentStep() const { return currentStep_.load(); }
    double getPositionBeats() const;

    // Pattern Management
    void setPattern(int index);
    int getCurrentPattern() const { return currentPattern_.load(); }
    Pattern& getPattern(int index);
    int getNumPatterns() const { return static_cast<int>(patterns_.size()); }
    void addPattern();

    // Step Sequencer Controls
    void setStep(int channel, int step, bool active);
    void setStepNote(int channel, int step, int note);
    void setStepVelocity(int channel, int step, float velocity);
    bool getStep(int channel, int step) const;

    // Get channel info for UI sync
    int getNumChannels() const;
    std::string getChannelName(int channel) const;

    // Channel Controls
    void setChannelVolume(int channel, float volume);
    void setChannelPan(int channel, float pan);
    void setChannelMute(int channel, bool muted);
    void setChannelSolo(int channel, bool soloed);
    void setChannelWaveform(int channel, Waveform waveform);
    int addChannel(const std::string& name = "New Channel");

    // Direct Note Trigger (for previewing)
    void noteOn(int channel, int note, float velocity = 0.8f);
    void noteOff(int channel);

    // Metering
    float getLeftLevel() const { return leftLevel_.load(); }
    float getRightLevel() const { return rightLevel_.load(); }
    float getCPUUsage() const { return cpuUsage_.load(); }

    // Callback for UI updates
    void setOnStepCallback(std::function<void(int step)> callback);

private:
    // SDL Audio
    SDL_AudioDeviceID audioDevice_{0};
    SDL_AudioSpec audioSpec_{};

    // Transport State
    std::atomic<bool> isPlaying_{false};
    std::atomic<bool> isPaused_{false};
    std::atomic<double> bpm_{120.0};
    std::atomic<int> currentStep_{0};
    std::atomic<int> currentPattern_{0};

    // Timing
    double samplesPerStep_{0.0};
    double sampleCounter_{0.0};

    // Patterns (thread-safe access via mutex)
    std::vector<Pattern> patterns_;
    mutable std::mutex patternMutex_;

    // Metering
    std::atomic<float> leftLevel_{0.0f};
    std::atomic<float> rightLevel_{0.0f};
    std::atomic<float> cpuUsage_{0.0f};

    // Callbacks
    std::function<void(int)> onStepCallback_;

    // Audio Callback
    static void audioCallback(void* userdata, Uint8* stream, int len);
    void processAudio(float* output, int numSamples);

    // Voice Processing
    float processVoice(SynthVoice& voice, double sampleRate);
    void triggerStep(int step);
    void updateEnvelope(SynthVoice& voice);

    // Utility
    double noteToFrequency(int note) const;
    void createDemoPatterns();
};

} // namespace daw::ui::imgui
