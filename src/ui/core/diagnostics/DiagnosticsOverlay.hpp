/**
 * @file DiagnosticsOverlay.hpp
 * @brief Live metrics, profiling overlay, and trace export
 */
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace daw::ui::diagnostics
{

/**
 * @brief Single timing event for profiling
 */
struct TimingEvent
{
    std::string name;
    std::string category;
    uint64_t startTimeUs{0};  // Microseconds since epoch
    uint64_t durationUs{0};
    int threadId{0};
    std::string args;  // JSON args for Chrome trace format
};

/**
 * @brief Frame timing statistics
 */
struct FrameStats
{
    float frameTimeMs{0.0f};
    float cpuTimeMs{0.0f};
    float gpuTimeMs{0.0f};  // If available
    int drawCalls{0};
    int vertexCount{0};
    int triangleCount{0};
    std::size_t allocatedBytes{0};
    float audioThreadOccupancy{0.0f};  // 0-1 of audio buffer time used
    int dirtySignals{0};
    int visibleNotes{0};
    int visibleClips{0};
};

/**
 * @brief Undo action record for introspection
 */
struct UndoRecord
{
    uint64_t id{0};
    std::string description;
    std::string timestamp;
    bool canUndo{true};
    bool canRedo{false};
};

/**
 * @brief Scoped timer for profiling sections
 */
class ScopedTimer
{
public:
    ScopedTimer(const std::string& name, const std::string& category = "UI");
    ~ScopedTimer();

    // Non-copyable
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    std::string name_;
    std::string category_;
    std::chrono::high_resolution_clock::time_point startTime_;
};

/**
 * @brief Diagnostics and profiling manager
 */
class DiagnosticsManager
{
public:
    DiagnosticsManager();
    ~DiagnosticsManager() = default;

    /**
     * @brief Begin a new frame
     */
    void beginFrame();

    /**
     * @brief End current frame and calculate stats
     */
    void endFrame();

    /**
     * @brief Record a timing event
     */
    void recordEvent(const TimingEvent& event);

    /**
     * @brief Record audio thread timing
     * @param occupancy Audio buffer usage ratio (0-1)
     */
    void recordAudioTiming(float occupancy);

    /**
     * @brief Get current frame stats
     */
    [[nodiscard]] const FrameStats& getCurrentStats() const { return currentStats_; }

    /**
     * @brief Get FPS
     */
    [[nodiscard]] float getFPS() const { return fps_; }

    /**
     * @brief Get average frame time (ms)
     */
    [[nodiscard]] float getAverageFrameTime() const { return avgFrameTimeMs_; }

    /**
     * @brief Get 99th percentile frame time (ms)
     */
    [[nodiscard]] float get99thPercentileFrameTime() const { return p99FrameTimeMs_; }

    /**
     * @brief Start trace capture
     */
    void startTraceCapture();

    /**
     * @brief Stop trace capture
     */
    void stopTraceCapture();

    /**
     * @brief Check if capturing
     */
    [[nodiscard]] bool isCapturing() const { return capturing_; }

    /**
     * @brief Export captured trace to Chrome trace format JSON
     * @param filepath Output file path
     * @return true if successful
     */
    bool exportTrace(const std::string& filepath) const;

    /**
     * @brief Get captured trace as JSON string
     */
    [[nodiscard]] std::string getTraceJSON() const;

    /**
     * @brief Clear captured events
     */
    void clearTrace();

    /**
     * @brief Get frame time history
     */
    [[nodiscard]] const std::deque<float>& getFrameTimeHistory() const { return frameTimeHistory_; }

    /**
     * @brief Get max history size
     */
    [[nodiscard]] std::size_t getHistorySize() const { return historySize_; }
    void setHistorySize(std::size_t size) { historySize_ = size; }

    /**
     * @brief Update draw call count for current frame
     */
    void setDrawCalls(int count) { currentStats_.drawCalls = count; }

    /**
     * @brief Update vertex count for current frame
     */
    void setVertexCount(int count) { currentStats_.vertexCount = count; }

    /**
     * @brief Update visible note count
     */
    void setVisibleNotes(int count) { currentStats_.visibleNotes = count; }

    /**
     * @brief Update visible clip count
     */
    void setVisibleClips(int count) { currentStats_.visibleClips = count; }

    /**
     * @brief Update dirty signal count
     */
    void setDirtySignals(int count) { currentStats_.dirtySignals = count; }

    /**
     * @brief Undo stack introspection
     */
    void pushUndoRecord(const UndoRecord& record);
    [[nodiscard]] const std::vector<UndoRecord>& getUndoHistory() const { return undoHistory_; }
    void clearUndoHistory() { undoHistory_.clear(); }

private:
    // Frame timing
    std::chrono::high_resolution_clock::time_point frameStartTime_;
    FrameStats currentStats_;
    std::deque<float> frameTimeHistory_;
    std::size_t historySize_{120};  // 2 seconds at 60fps

    // Calculated metrics
    float fps_{0.0f};
    float avgFrameTimeMs_{0.0f};
    float p99FrameTimeMs_{0.0f};

    // Trace capture
    std::atomic<bool> capturing_{false};
    std::vector<TimingEvent> capturedEvents_;
    mutable std::mutex traceMutex_;

    // Undo introspection
    std::vector<UndoRecord> undoHistory_;

    void updateMetrics();
};

/**
 * @brief Global diagnostics manager instance
 */
DiagnosticsManager& getGlobalDiagnostics();

/**
 * @brief Diagnostics overlay panel for ImGui
 */
class DiagnosticsOverlay
{
public:
    DiagnosticsOverlay() = default;

    /**
     * @brief Draw the overlay
     * @param visible Reference to visibility flag
     * @param diagnostics Reference to diagnostics manager
     */
    void draw(bool& visible, DiagnosticsManager& diagnostics);

    /**
     * @brief Settings
     */
    bool showGraph{true};
    bool showDetails{false};
    bool showTrace{false};
    bool showUndoHistory{false};
    float graphHeight{80.0f};
    float overlayAlpha{0.85f};

private:
    void drawFrameTimeGraph(DiagnosticsManager& diagnostics);
    void drawMetricsDetails(DiagnosticsManager& diagnostics);
    void drawTraceControls(DiagnosticsManager& diagnostics);
    void drawUndoIntrospection(DiagnosticsManager& diagnostics);
};

/**
 * @brief Helper macro for scoped timing
 */
#define DAW_PROFILE_SCOPE(name) \
    daw::ui::diagnostics::ScopedTimer _timer_##__LINE__(name)

#define DAW_PROFILE_SCOPE_CAT(name, category) \
    daw::ui::diagnostics::ScopedTimer _timer_##__LINE__(name, category)

} // namespace daw::ui::diagnostics
