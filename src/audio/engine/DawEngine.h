#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Transport.h"
#include "AudioGraph.h"
#include "../../core/utilities/PerformanceMonitor.h"
#include <memory>
#include <atomic>
#include <vector>
#include <chrono>
#include <cstdint>

namespace daw::audio::engine
{

/**
 * @brief Main DAW engine
 * 
 * Integrates with AudioDeviceManager, manages Transport and AudioGraph.
 * Provides thread-safe APIs for UI and project model.
 * Follows DAW_DEV_RULES: real-time safe, lock-free communication.
 */
class DawEngine
{
public:
    DawEngine();
    ~DawEngine();

    // Non-copyable, non-movable (device manager cannot be moved)
    DawEngine(const DawEngine&) = delete;
    DawEngine& operator=(const DawEngine&) = delete;

    // Initialization (call from UI thread)
    bool initialise();
    void shutdown();

    // Audio callbacks (called from audio thread)
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) noexcept;

    // Transport control (call from UI/project thread, NOT audio thread)
    void play();
    void stop();
    void setPositionInBeats(double positionBeats);
    void setTempo(double bpm);
    void setTimeSignature(int numerator, int denominator);

    // Transport queries (safe from any thread)
    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] double getPositionInBeats() const;
    [[nodiscard]] double getTempo() const;
    [[nodiscard]] int getTimeSignatureNumerator() const;
    [[nodiscard]] int getTimeSignatureDenominator() const;

    // Track management (call from UI/project thread, NOT audio thread)
    int addTrack();
    void removeTrack(int trackIndex);
    void setTrackGain(int trackIndex, float gainDb);
    void setTrackPan(int trackIndex, float pan);
    void setTrackMute(int trackIndex, bool muted);
    void setTrackSolo(int trackIndex, bool soloed);
    [[nodiscard]] int getNumTracks() const;

    // Metering (lock-free, safe to call from UI thread)
    struct MeterData
    {
        float peak;
        float rms;
    };
    [[nodiscard]] MeterData getTrackMeter(int trackIndex) const;
    [[nodiscard]] MeterData getMasterMeter() const;

    // CPU load (safe to call from UI thread)
    [[nodiscard]] float getCpuLoad() const;
    [[nodiscard]] float getCpuLoadPercent() const;
    [[nodiscard]] uint64_t getXrunCount() const;
    void resetXrunCount();

    // Performance metrics
    [[nodiscard]] std::chrono::nanoseconds getP50ProcessTime() const;
    [[nodiscard]] std::chrono::nanoseconds getP95ProcessTime() const;
    [[nodiscard]] std::chrono::nanoseconds getP99ProcessTime() const;

    // AudioDeviceManager access
    [[nodiscard]] juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

private:
    juce::AudioDeviceManager deviceManager;
    std::unique_ptr<Transport> transport;
    std::unique_ptr<AudioGraph> audioGraph;
    
    // Performance monitoring
    daw::core::utilities::PerformanceMonitor performanceMonitor;
    
    // Legacy CPU load (kept for compatibility)
    std::atomic<float> cpuLoad;
    std::chrono::high_resolution_clock::time_point lastProcessTime;
    std::chrono::high_resolution_clock::duration accumulatedProcessTime;
    int processBlockCount;
    
    void updateCpuLoad(std::chrono::high_resolution_clock::duration processTime, int numSamples, double sampleRate);
    
    // Audio callback wrapper
    class AudioCallback;
    std::unique_ptr<AudioCallback> audioCallback;
};

} // namespace daw::audio::engine

