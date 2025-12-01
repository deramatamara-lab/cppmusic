#pragma once
/**
 * @file ArrangementAnalyzer.hpp
 * @brief Arrangement structure analysis for AI suggestions.
 */

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cppmusic::ai::arrangement {

/**
 * @brief Section type in arrangement.
 */
enum class SectionType {
    Intro,
    Verse,
    PreChorus,
    Chorus,
    Bridge,
    Breakdown,
    Buildup,
    Drop,
    Outro,
    Unknown
};

/**
 * @brief Detected section boundary.
 */
struct SectionBoundary {
    double startBeat = 0.0;
    double endBeat = 0.0;
    SectionType type = SectionType::Unknown;
    float confidence = 0.0f;
    
    /**
     * @brief Get section duration in beats.
     */
    [[nodiscard]] double getDuration() const noexcept {
        return endBeat - startBeat;
    }
};

/**
 * @brief Arrangement metrics for analysis.
 */
struct ArrangementMetrics {
    // Structure metrics
    float structuralClarity = 0.0f;   ///< How clear are section boundaries (0-1)
    float formConsistency = 0.0f;     ///< How consistent is the form (0-1)
    
    // Variation metrics
    float melodicVariation = 0.0f;    ///< Variation in melodic content (0-1)
    float rhythmicVariation = 0.0f;   ///< Variation in rhythmic patterns (0-1)
    float timbreVariation = 0.0f;     ///< Variation in instrumentation (0-1)
    
    // Energy metrics
    float energyRange = 0.0f;         ///< Dynamic range of energy (0-1)
    float buildupIntensity = 0.0f;    ///< Strength of buildups (0-1)
    
    // Overall scores
    float interestScore = 0.0f;       ///< Overall musical interest (0-1)
    float coherenceScore = 0.0f;      ///< Overall structural coherence (0-1)
};

/**
 * @brief Energy curve point.
 */
struct EnergyPoint {
    double beat = 0.0;
    float energy = 0.0f;
};

/**
 * @brief Full arrangement analysis result.
 */
struct ArrangementAnalysis {
    std::vector<SectionBoundary> sections;
    ArrangementMetrics metrics;
    std::vector<EnergyPoint> energyCurve;
    
    std::optional<int> detectedKey;
    std::optional<float> detectedTempo;
    
    /**
     * @brief Get the section at a given beat.
     */
    [[nodiscard]] const SectionBoundary* getSectionAt(double beat) const;
    
    /**
     * @brief Get energy at a given beat.
     */
    [[nodiscard]] float getEnergyAt(double beat) const;
};

/**
 * @brief Analyzes arrangement structure.
 */
class ArrangementAnalyzer {
public:
    ArrangementAnalyzer();
    ~ArrangementAnalyzer();
    
    // =========================================================================
    // Analysis
    // =========================================================================
    
    /**
     * @brief Analyze energy curve from note density/velocity.
     * @param beats Beat positions of notes.
     * @param velocities Note velocities.
     * @param totalBeats Total arrangement length in beats.
     */
    [[nodiscard]] std::vector<EnergyPoint> analyzeEnergy(
        const std::vector<double>& beats,
        const std::vector<float>& velocities,
        double totalBeats) const;
    
    /**
     * @brief Detect section boundaries from energy curve.
     */
    [[nodiscard]] std::vector<SectionBoundary> detectSections(
        const std::vector<EnergyPoint>& energyCurve) const;
    
    /**
     * @brief Classify section type based on context.
     */
    [[nodiscard]] SectionType classifySection(
        const std::vector<EnergyPoint>& energyCurve,
        double startBeat, double endBeat,
        std::size_t sectionIndex, std::size_t totalSections) const;
    
    /**
     * @brief Compute arrangement metrics.
     */
    [[nodiscard]] ArrangementMetrics computeMetrics(
        const std::vector<SectionBoundary>& sections,
        const std::vector<EnergyPoint>& energyCurve) const;
    
    /**
     * @brief Compute variation score between two sections.
     */
    [[nodiscard]] float computeSectionDifference(
        const std::vector<EnergyPoint>& energyCurve,
        const SectionBoundary& section1,
        const SectionBoundary& section2) const;
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Set minimum section length in beats.
     */
    void setMinSectionLength(double beats);
    
    /**
     * @brief Set energy change threshold for section detection.
     */
    void setEnergyChangeThreshold(float threshold);
    
private:
    double minSectionLength_ = 16.0;     // 4 bars at 4/4
    float energyChangeThreshold_ = 0.2f;
    std::size_t energyWindowSize_ = 4;   // Beats for energy averaging
};

/**
 * @brief Get string representation of section type.
 */
[[nodiscard]] const char* toString(SectionType type);

} // namespace cppmusic::ai::arrangement
