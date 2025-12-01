#pragma once
/**
 * @file Scheduler.hpp
 * @brief Sample-accurate beat-to-frame conversion with warp transformation support.
 *
 * Provides scheduling infrastructure for the cppmusic DAW engine with:
 * - Sample-accurate beat to frame conversion
 * - Tempo and time signature handling
 * - Polymeter support (independent pattern lengths)
 * - Warp transformations (piecewise linear time mapping)
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace cppmusic::engine {

/**
 * @brief A warp marker for non-linear time mapping.
 *
 * Warp markers define control points for piecewise linear time mapping,
 * allowing for time stretching/compression of musical content.
 */
struct WarpMarker {
    double sourceBeat{0.0};    ///< Beat position in source timeline
    double targetBeat{0.0};    ///< Beat position in warped timeline
    
    bool operator<(const WarpMarker& other) const noexcept {
        return sourceBeat < other.sourceBeat;
    }
};

/**
 * @brief Warp map for non-linear time transformations.
 *
 * Implements piecewise linear time mapping evaluated at event extraction stage.
 * Used for clip warping, time stretch, and fit-to-tempo operations.
 */
class WarpMap {
public:
    WarpMap() = default;
    ~WarpMap() = default;
    
    WarpMap(const WarpMap&) = default;
    WarpMap& operator=(const WarpMap&) = default;
    WarpMap(WarpMap&&) noexcept = default;
    WarpMap& operator=(WarpMap&&) noexcept = default;

    /**
     * @brief Add a warp marker to the map.
     * @param marker The warp marker to add.
     */
    void addMarker(const WarpMarker& marker) {
        markers_.push_back(marker);
        std::sort(markers_.begin(), markers_.end());
    }

    /**
     * @brief Remove a warp marker at the specified index.
     */
    bool removeMarker(std::size_t index) {
        if (index >= markers_.size()) {
            return false;
        }
        markers_.erase(markers_.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    /**
     * @brief Clear all warp markers.
     */
    void clearMarkers() noexcept {
        markers_.clear();
    }

    /**
     * @brief Get all warp markers.
     */
    [[nodiscard]] const std::vector<WarpMarker>& getMarkers() const noexcept {
        return markers_;
    }

    /**
     * @brief Check if warp map is active (has at least 2 markers).
     */
    [[nodiscard]] bool isActive() const noexcept {
        return markers_.size() >= 2;
    }

    /**
     * @brief Convert source beat to warped beat using piecewise linear interpolation.
     * @param sourceBeat Beat position in source timeline.
     * @return Beat position in warped timeline.
     */
    [[nodiscard]] double sourceToTarget(double sourceBeat) const noexcept {
        if (markers_.size() < 2) {
            return sourceBeat;  // No warping
        }

        // Find the segment containing sourceBeat
        for (std::size_t i = 1; i < markers_.size(); ++i) {
            if (sourceBeat <= markers_[i].sourceBeat) {
                // Linear interpolation within segment
                const auto& m0 = markers_[i - 1];
                const auto& m1 = markers_[i];
                
                double sourceRange = m1.sourceBeat - m0.sourceBeat;
                if (sourceRange <= 0.0) {
                    return m0.targetBeat;
                }
                
                double t = (sourceBeat - m0.sourceBeat) / sourceRange;
                return m0.targetBeat + t * (m1.targetBeat - m0.targetBeat);
            }
        }

        // Beyond last marker - extrapolate using last segment's slope
        if (markers_.size() >= 2) {
            const auto& m0 = markers_[markers_.size() - 2];
            const auto& m1 = markers_[markers_.size() - 1];
            
            double sourceRange = m1.sourceBeat - m0.sourceBeat;
            if (sourceRange > 0.0) {
                double slope = (m1.targetBeat - m0.targetBeat) / sourceRange;
                return m1.targetBeat + slope * (sourceBeat - m1.sourceBeat);
            }
        }
        
        return sourceBeat;
    }

    /**
     * @brief Convert warped beat back to source beat (inverse mapping).
     * @param targetBeat Beat position in warped timeline.
     * @return Beat position in source timeline.
     */
    [[nodiscard]] double targetToSource(double targetBeat) const noexcept {
        if (markers_.size() < 2) {
            return targetBeat;
        }

        // Find the segment containing targetBeat
        for (std::size_t i = 1; i < markers_.size(); ++i) {
            double minTarget = std::min(markers_[i - 1].targetBeat, markers_[i].targetBeat);
            double maxTarget = std::max(markers_[i - 1].targetBeat, markers_[i].targetBeat);
            
            if (targetBeat >= minTarget && targetBeat <= maxTarget) {
                const auto& m0 = markers_[i - 1];
                const auto& m1 = markers_[i];
                
                double targetRange = m1.targetBeat - m0.targetBeat;
                if (std::abs(targetRange) <= 1e-9) {
                    return m0.sourceBeat;
                }
                
                double t = (targetBeat - m0.targetBeat) / targetRange;
                return m0.sourceBeat + t * (m1.sourceBeat - m0.sourceBeat);
            }
        }

        // Beyond markers - extrapolate
        const auto& m0 = markers_[markers_.size() - 2];
        const auto& m1 = markers_[markers_.size() - 1];
        
        double targetRange = m1.targetBeat - m0.targetBeat;
        if (std::abs(targetRange) > 1e-9) {
            double slope = (m1.sourceBeat - m0.sourceBeat) / targetRange;
            return m1.sourceBeat + slope * (targetBeat - m1.targetBeat);
        }
        
        return targetBeat;
    }

private:
    std::vector<WarpMarker> markers_;
};

/**
 * @brief Sample-accurate scheduler for beat-to-frame conversion.
 *
 * Handles tempo, time signature, and provides utilities for converting
 * between beats and audio frames (samples). Supports polymeter patterns
 * with independent lengths from the global bar structure.
 */
class Scheduler {
public:
    Scheduler() noexcept = default;
    ~Scheduler() = default;
    
    // Non-copyable to prevent accidental state duplication in audio thread
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler(Scheduler&&) = default;
    Scheduler& operator=(Scheduler&&) = default;

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * @brief Set the sample rate.
     * @param sampleRate Sample rate in Hz.
     */
    void setSampleRate(double sampleRate) noexcept {
        sampleRate_ = std::max(1.0, sampleRate);
    }

    /**
     * @brief Get the sample rate.
     */
    [[nodiscard]] double getSampleRate() const noexcept {
        return sampleRate_;
    }

    /**
     * @brief Set the tempo in BPM.
     * @param bpm Beats per minute (clamped to [20, 999]).
     */
    void setTempo(double bpm) noexcept {
        tempo_ = std::max(20.0, std::min(999.0, bpm));
    }

    /**
     * @brief Get the tempo in BPM.
     */
    [[nodiscard]] double getTempo() const noexcept {
        return tempo_;
    }

    /**
     * @brief Set the time signature.
     * @param numerator Beats per bar (1-32).
     * @param denominator Beat note value (1-32).
     */
    void setTimeSignature(int numerator, int denominator) noexcept {
        timeSignatureNum_ = std::max(1, std::min(32, numerator));
        timeSignatureDenom_ = std::max(1, std::min(32, denominator));
    }

    /**
     * @brief Get time signature numerator.
     */
    [[nodiscard]] int getTimeSignatureNumerator() const noexcept {
        return timeSignatureNum_;
    }

    /**
     * @brief Get time signature denominator.
     */
    [[nodiscard]] int getTimeSignatureDenominator() const noexcept {
        return timeSignatureDenom_;
    }

    // =========================================================================
    // Beat/Frame Conversion
    // =========================================================================

    /**
     * @brief Convert beats to frames (samples).
     * @param beats Number of beats.
     * @return Number of audio frames.
     */
    [[nodiscard]] std::int64_t beatsToFrames(double beats) const noexcept {
        double samplesPerBeat = (60.0 / tempo_) * sampleRate_;
        return static_cast<std::int64_t>(std::round(beats * samplesPerBeat));
    }

    /**
     * @brief Convert frames (samples) to beats.
     * @param frames Number of audio frames.
     * @return Number of beats.
     */
    [[nodiscard]] double framesToBeats(std::int64_t frames) const noexcept {
        double samplesPerBeat = (60.0 / tempo_) * sampleRate_;
        return static_cast<double>(frames) / samplesPerBeat;
    }

    /**
     * @brief Get samples per beat at current tempo.
     */
    [[nodiscard]] double getSamplesPerBeat() const noexcept {
        return (60.0 / tempo_) * sampleRate_;
    }

    /**
     * @brief Get samples per bar at current tempo and time signature.
     */
    [[nodiscard]] double getSamplesPerBar() const noexcept {
        return getSamplesPerBeat() * static_cast<double>(timeSignatureNum_);
    }

    /**
     * @brief Convert bars to beats.
     */
    [[nodiscard]] double barsToBeats(double bars) const noexcept {
        return bars * static_cast<double>(timeSignatureNum_);
    }

    /**
     * @brief Convert beats to bars.
     */
    [[nodiscard]] double beatsToBar(double beats) const noexcept {
        return beats / static_cast<double>(timeSignatureNum_);
    }

    // =========================================================================
    // Polymeter Support
    // =========================================================================

    /**
     * @brief Calculate loop position for a pattern with independent length.
     * @param globalBeat Current global beat position.
     * @param patternLengthBeats Length of the pattern in beats.
     * @return Beat position within the pattern (looped).
     */
    [[nodiscard]] static double getPatternBeat(double globalBeat, double patternLengthBeats) noexcept {
        if (patternLengthBeats <= 0.0) {
            return 0.0;
        }
        double beat = std::fmod(globalBeat, patternLengthBeats);
        // Handle negative beats (when playing before pattern start)
        if (beat < 0.0) {
            beat += patternLengthBeats;
        }
        return beat;
    }

    /**
     * @brief Calculate loop iteration for a pattern.
     * @param globalBeat Current global beat position.
     * @param patternLengthBeats Length of the pattern in beats.
     * @return Zero-indexed loop iteration number.
     */
    [[nodiscard]] static std::uint32_t getLoopIteration(double globalBeat, double patternLengthBeats) noexcept {
        if (patternLengthBeats <= 0.0) {
            return 0;
        }
        return static_cast<std::uint32_t>(std::max(0.0, std::floor(globalBeat / patternLengthBeats)));
    }

    // =========================================================================
    // Micro-Timing
    // =========================================================================

    /**
     * @brief Apply micro-timing offset to a frame position.
     * @param framePosition Base frame position.
     * @param microOffset Offset in samples (can be negative).
     * @return Adjusted frame position (clamped to >= 0).
     */
    [[nodiscard]] static std::int64_t applyMicroTiming(
        std::int64_t framePosition, 
        std::int32_t microOffset) noexcept {
        std::int64_t adjusted = framePosition + static_cast<std::int64_t>(microOffset);
        return std::max(static_cast<std::int64_t>(0), adjusted);
    }

private:
    double sampleRate_{44100.0};
    double tempo_{120.0};
    int timeSignatureNum_{4};
    int timeSignatureDenom_{4};
};

} // namespace cppmusic::engine
