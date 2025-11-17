#pragma once

#include "DawEngine.h"
#include <memory>
#include <vector>

namespace daw::audio::engine
{

/**
 * @brief Engine context facade for UI communication
 * 
 * Provides a clean interface for UI components to interact with the engine.
 * Separates UI concerns from engine implementation.
 * Follows DAW_DEV_RULES: clear separation of layers.
 */
class EngineContext
{
public:
    explicit EngineContext();
    ~EngineContext();

    // Non-copyable, movable
    EngineContext(const EngineContext&) = delete;
    EngineContext& operator=(const EngineContext&) = delete;
    EngineContext(EngineContext&&) noexcept = default;
    EngineContext& operator=(EngineContext&&) noexcept = default;

    // Initialization
    bool initialise();
    void shutdown();

    // Transport control (call from UI thread)
    void play();
    void stop();
    void setPositionInBeats(double positionBeats);
    void setTempo(double bpm);
    void setTimeSignature(int numerator, int denominator);

    // Transport queries (safe from UI thread)
    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] double getPositionInBeats() const;
    [[nodiscard]] double getTempo() const;
    [[nodiscard]] int getTimeSignatureNumerator() const;
    [[nodiscard]] int getTimeSignatureDenominator() const;

    // Track management (call from UI thread)
    int addTrack();
    void removeTrack(int trackIndex);
    void setTrackGain(int trackIndex, float gainDb);
    void setTrackPan(int trackIndex, float pan);
    void setTrackMute(int trackIndex, bool muted);
    void setTrackSolo(int trackIndex, bool soloed);
    [[nodiscard]] int getNumTracks() const;

    // Metering (lock-free, safe from UI thread)
    using MeterData = DawEngine::MeterData;
    [[nodiscard]] MeterData getTrackMeter(int trackIndex) const;
    [[nodiscard]] MeterData getMasterMeter() const;

    // CPU load (safe from UI thread)
    [[nodiscard]] float getCpuLoad() const;

private:
    std::unique_ptr<DawEngine> engine;
};

} // namespace daw::audio::engine

