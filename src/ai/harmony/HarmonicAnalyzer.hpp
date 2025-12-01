#pragma once
/**
 * @file HarmonicAnalyzer.hpp
 * @brief Harmonic analysis including pitch class vectors and tension metrics.
 */

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cppmusic::ai::harmony {

/**
 * @brief 12-dimensional pitch class vector.
 */
struct PitchClassVector {
    std::array<float, 12> values{};  ///< C, C#, D, D#, E, F, F#, G, G#, A, A#, B
    
    /**
     * @brief Normalize so values sum to 1.
     */
    void normalize();
    
    /**
     * @brief Get indices of dominant pitch classes.
     * @param count Number of top pitch classes to return.
     */
    [[nodiscard]] std::vector<int> getDominant(int count = 3) const;
    
    /**
     * @brief Clear all values to zero.
     */
    void clear();
    
    /**
     * @brief Add a pitch class with given weight.
     */
    void addPitch(int midiNote, float weight = 1.0f);
};

/**
 * @brief Recognized chord type.
 */
enum class ChordQuality {
    Major,
    Minor,
    Diminished,
    Augmented,
    Dominant7,
    Major7,
    Minor7,
    Diminished7,
    HalfDiminished7,
    Sus2,
    Sus4,
    Unknown
};

/**
 * @brief Detected chord information.
 */
struct ChordInfo {
    int rootPitchClass = 0;        ///< Root note (0 = C)
    ChordQuality quality = ChordQuality::Unknown;
    float confidence = 0.0f;       ///< Detection confidence (0-1)
    std::vector<int> extensions;   ///< Additional notes (9, 11, 13)
    
    /**
     * @brief Get chord name (e.g., "Cmaj7", "F#m").
     */
    [[nodiscard]] std::string getName() const;
    
    /**
     * @brief Get Roman numeral in given key.
     */
    [[nodiscard]] std::string getRomanNumeral(int keyRoot, bool isMinor) const;
};

/**
 * @brief Key detection result.
 */
struct KeyInfo {
    int rootPitchClass = 0;        ///< Key root (0 = C)
    bool isMinor = false;          ///< true for minor, false for major
    float confidence = 0.0f;       ///< Detection confidence (0-1)
    
    /**
     * @brief Get key name (e.g., "C major", "A minor").
     */
    [[nodiscard]] std::string getName() const;
};

/**
 * @brief Harmonic analyzer for chord and tension analysis.
 */
class HarmonicAnalyzer {
public:
    HarmonicAnalyzer();
    ~HarmonicAnalyzer();
    
    // =========================================================================
    // Pitch Class Analysis
    // =========================================================================
    
    /**
     * @brief Create pitch class vector from MIDI notes.
     * @param notes MIDI note numbers (0-127).
     * @param velocities Note velocities for weighting (optional).
     */
    [[nodiscard]] PitchClassVector createPCV(
        const std::vector<std::uint8_t>& notes,
        const std::vector<std::uint8_t>& velocities = {}) const;
    
    // =========================================================================
    // Tension Analysis
    // =========================================================================
    
    /**
     * @brief Compute harmonic tension metric.
     * @param pcv Pitch class vector.
     * @return Tension value (0 = consonant, 1 = dissonant).
     */
    [[nodiscard]] float computeTension(const PitchClassVector& pcv) const;
    
    /**
     * @brief Compute tension between two chords.
     * @param current Current chord PCV.
     * @param next Next chord PCV.
     * @return Transition tension (0 = smooth, 1 = harsh).
     */
    [[nodiscard]] float computeTransitionTension(
        const PitchClassVector& current,
        const PitchClassVector& next) const;
    
    // =========================================================================
    // Chord Detection
    // =========================================================================
    
    /**
     * @brief Detect chord from pitch class vector.
     */
    [[nodiscard]] ChordInfo detectChord(const PitchClassVector& pcv) const;
    
    /**
     * @brief Detect chord from MIDI notes.
     */
    [[nodiscard]] ChordInfo detectChord(const std::vector<std::uint8_t>& notes) const;
    
    // =========================================================================
    // Key Detection
    // =========================================================================
    
    /**
     * @brief Detect key from pitch class distribution.
     */
    [[nodiscard]] KeyInfo detectKey(const PitchClassVector& pcv) const;
    
    /**
     * @brief Detect key from chord sequence.
     */
    [[nodiscard]] KeyInfo detectKeyFromChords(const std::vector<ChordInfo>& chords) const;
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Set tension weights.
     */
    void setTensionWeights(float minorSecond, float tritone, 
                           float perfectFifth, float density);
    
private:
    float weightMinorSecond_ = 0.3f;
    float weightTritone_ = 0.2f;
    float weightPerfectFifth_ = 0.15f;
    float weightDensity_ = 0.1f;
};

/**
 * @brief Get pitch class name.
 */
[[nodiscard]] const char* getPitchClassName(int pitchClass);

/**
 * @brief Get interval name.
 */
[[nodiscard]] const char* getIntervalName(int semitones);

} // namespace cppmusic::ai::harmony
