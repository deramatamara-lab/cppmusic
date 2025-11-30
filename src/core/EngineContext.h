#pragma once

#include <juce_core/juce_core.h>
#include "RealtimeMessageQueue.h"
#include <atomic>
#include <memory>
#include <functional>
#include <cstdint>

namespace daw::core {

/**
 * Thread-safe facade that provides controlled access to the audio engine.
 *
 * This class enforces the architectural rule that "UI only talks to engine/model
 * via well-defined interfaces" by providing a message-based API that never
 * directly touches the audio thread.
 *
 * Key responsibilities:
 * - Parameter changes from UI → Audio (via lock-free queue)
 * - Metering data from Audio → UI (via lock-free queue)
 * - Transport control with sample-accurate timing
 * - AI result delivery without blocking audio thread
 * - Thread-safe state queries
 */
class EngineContext
{
public:
    /**
     * Engine state that can be safely queried from any thread
     */
    struct EngineState
    {
        std::atomic<bool> isPlaying{false};
        std::atomic<bool> isRecording{false};
        std::atomic<double> currentPosition{0.0}; // seconds
        std::atomic<double> sampleRate{44100.0};
        std::atomic<int> bufferSize{512};
        std::atomic<float> cpuUsage{0.0f}; // 0.0 to 1.0
        std::atomic<uint32_t> xrunCount{0};
    };

    /**
     * Performance metrics updated by audio thread
     */
    struct PerformanceMetrics
    {
        std::atomic<float> averageLoad{0.0f};
        std::atomic<float> peakLoad{0.0f};
        std::atomic<uint64_t> samplesProcessed{0};
        std::atomic<uint32_t> callbackCount{0};
        std::atomic<uint64_t> lastCallbackTime{0}; // microseconds
    };

    EngineContext();
    ~EngineContext();

    //==============================================================================
    // Transport Control
    //==============================================================================

    /**
     * Start playback (thread-safe)
     */
    void play();

    /**
     * Stop playback (thread-safe)
     */
    void stop();

    /**
     * Pause playback (thread-safe)
     */
    void pause();

    /**
     * Start recording (thread-safe)
     */
    void record();

    /**
     * Set playback position (thread-safe)
     * @param positionSeconds Target position in seconds
     */
    void setPosition(double positionSeconds);

    //==============================================================================
    // Parameter Control
    //==============================================================================

    /**
     * Set a parameter value (thread-safe)
     * @param parameterId Unique parameter identifier
     * @param value New parameter value
     * @return true if parameter was queued, false if queue was full
     */
    [[nodiscard]] bool setParameter(uint32_t parameterId, float value);

    /**
     * Get current parameter value (may be stale)
     * @param parameterId Parameter to query
     * @return Current cached value
     */
    [[nodiscard]] float getParameter(uint32_t parameterId) const;

    //==============================================================================
    // AI Integration
    //==============================================================================

    /**
     * Submit AI result to audio thread (thread-safe)
     * @param result AI processing result
     * @return true if result was queued, false if queue was full
     */
    [[nodiscard]] bool submitAIResult(const Messages::AIResult& result);

    /**
     * Register callback for AI results (called from UI thread)
     * @param callback Function to call when AI results are ready
     */
    void setAIResultCallback(std::function<void(const Messages::AIResult&)> callback);

    //==============================================================================
    // Metering & Monitoring
    //==============================================================================

    /**
     * Get latest meter readings (thread-safe)
     * @param channelId Channel to query
     * @return Latest meter data, or default if no data available
     */
    [[nodiscard]] Messages::MeterUpdate getMeterData(uint32_t channelId) const;

    /**
     * Register callback for meter updates (called from UI thread)
     * @param callback Function to call when meter data is available
     */
    void setMeterCallback(std::function<void(const Messages::MeterUpdate&)> callback);

    //==============================================================================
    // State Queries
    //==============================================================================

    /**
     * Get current engine state (thread-safe, atomic reads)
     */
    const EngineState& getEngineState() const { return engineState; }

    /**
     * Get performance metrics (thread-safe, atomic reads)
     */
    const PerformanceMetrics& getPerformanceMetrics() const { return performanceMetrics; }

    /**
     * Check if engine is initialized and ready
     */
    [[nodiscard]] bool isReady() const { return initialized.load(); }

    //==============================================================================
    // Internal Audio Thread Interface
    //==============================================================================

    /**
     * Process pending messages (called from audio thread only)
     * This method consumes queued commands and updates internal state
     */
    void processAudioThreadMessages() noexcept;

    /**
     * Update performance metrics (called from audio thread only)
     * @param cpuLoad Current CPU usage (0.0 to 1.0)
     * @param samplesProcessed Number of samples in this callback
     */
    void updatePerformanceMetrics(float cpuLoad, int samplesProcessed) noexcept;

    /**
     * Submit meter data (called from audio thread only)
     * @param meterData Meter reading to publish
     */
    void submitMeterData(const Messages::MeterUpdate& meterData) noexcept;

    //==============================================================================
    // Lifecycle
    //==============================================================================

    /**
     * Initialize the context (called once at startup)
     */
    void initialize();

    /**
     * Shutdown the context (called once at shutdown)
     */
    void shutdown();

private:
    std::atomic<bool> initialized{false};

    // Thread-safe state
    EngineState engineState;
    PerformanceMetrics performanceMetrics;

    // Lock-free communication queues
    std::unique_ptr<TransportQueue> transportQueue;
    std::unique_ptr<ParameterQueue> parameterQueue;
    std::unique_ptr<AIResultQueue> aiResultQueue;
    std::unique_ptr<MeterQueue> meterQueue;

    // UI callbacks (called from message thread)
    std::function<void(const Messages::AIResult&)> aiResultCallback;
    std::function<void(const Messages::MeterUpdate&)> meterCallback;

    // Parameter cache for UI queries
    mutable std::array<std::atomic<float>, 1024> parameterCache;

    // Meter data cache
    mutable std::array<std::atomic<Messages::MeterUpdate>, 64> meterCache;

    // Message processing
    void processTransportMessages() noexcept;
    void processParameterMessages() noexcept;
    void processAIMessages() noexcept;
    void processMeterMessages();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineContext)
};

} // namespace daw::core
