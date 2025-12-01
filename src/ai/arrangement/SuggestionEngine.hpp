#pragma once
/**
 * @file SuggestionEngine.hpp
 * @brief AI-powered arrangement suggestion generation.
 */

#include "ArrangementAnalyzer.hpp"
#include <functional>
#include <string>
#include <vector>

namespace cppmusic::ai::arrangement {

/**
 * @brief Type of suggestion.
 */
enum class SuggestionType {
    PatternPlacement,  ///< Suggest placing a pattern
    Transition,        ///< Suggest a transition type
    Fill,              ///< Suggest a fill/break
    Variation,         ///< Suggest varying existing content
    Energy             ///< Suggest energy adjustment
};

/**
 * @brief Pattern placement suggestion.
 */
struct PatternSuggestion {
    std::string patternName;
    double suggestedBeat = 0.0;
    int suggestedTrack = 0;
    float confidence = 0.0f;
    std::string rationale;
};

/**
 * @brief Transition type.
 */
enum class TransitionType {
    Cut,         ///< Hard cut
    Fade,        ///< Crossfade
    Buildup,     ///< Energy increase
    Breakdown,   ///< Energy decrease
    FilterSweep, ///< Filter automation
    Riser        ///< SFX-based transition
};

/**
 * @brief Transition suggestion.
 */
struct TransitionSuggestion {
    TransitionType type = TransitionType::Cut;
    double startBeat = 0.0;
    double endBeat = 0.0;
    float confidence = 0.0f;
    std::string rationale;
};

/**
 * @brief Fill/break suggestion.
 */
struct FillSuggestion {
    std::string description;
    double beat = 0.0;
    double duration = 0.0;
    float confidence = 0.0f;
    std::string rationale;
};

/**
 * @brief General suggestion wrapper.
 */
struct Suggestion {
    SuggestionType type = SuggestionType::PatternPlacement;
    double beat = 0.0;
    float confidence = 0.0f;
    std::string description;
    std::string rationale;
    
    // Type-specific data (one will be valid based on type)
    PatternSuggestion patternSuggestion;
    TransitionSuggestion transitionSuggestion;
    FillSuggestion fillSuggestion;
};

/**
 * @brief Generates arrangement suggestions based on analysis.
 */
class SuggestionEngine {
public:
    SuggestionEngine();
    ~SuggestionEngine();
    
    // =========================================================================
    // Suggestion Generation
    // =========================================================================
    
    /**
     * @brief Get all suggestions for current position.
     * @param analysis Arrangement analysis.
     * @param currentBeat Current playhead position.
     * @param existingPatterns Names of patterns already in project.
     */
    [[nodiscard]] std::vector<Suggestion> getSuggestions(
        const ArrangementAnalysis& analysis,
        double currentBeat,
        const std::vector<std::string>& existingPatterns = {}) const;
    
    /**
     * @brief Suggest patterns for a position.
     */
    [[nodiscard]] std::vector<PatternSuggestion> suggestPatterns(
        const ArrangementAnalysis& analysis,
        double beat,
        const std::vector<std::string>& existingPatterns = {}) const;
    
    /**
     * @brief Suggest transitions between sections.
     */
    [[nodiscard]] std::vector<TransitionSuggestion> suggestTransitions(
        const ArrangementAnalysis& analysis,
        double fromBeat, double toBeat) const;
    
    /**
     * @brief Suggest fills at section boundaries.
     */
    [[nodiscard]] std::vector<FillSuggestion> suggestFills(
        const ArrangementAnalysis& analysis,
        double beat) const;
    
    // =========================================================================
    // Scoring
    // =========================================================================
    
    /**
     * @brief Score a suggestion for relevance.
     */
    [[nodiscard]] float scoreSuggestion(
        const Suggestion& suggestion,
        const ArrangementAnalysis& analysis) const;
    
    /**
     * @brief Compute context fit score.
     */
    [[nodiscard]] float computeContextFit(
        const Suggestion& suggestion,
        const ArrangementAnalysis& analysis) const;
    
    /**
     * @brief Compute interest contribution score.
     */
    [[nodiscard]] float computeInterestContribution(
        const Suggestion& suggestion,
        const ArrangementAnalysis& analysis) const;
    
    // =========================================================================
    // Rationale Generation
    // =========================================================================
    
    /**
     * @brief Generate human-readable rationale for a suggestion.
     */
    [[nodiscard]] std::string generateRationale(
        const Suggestion& suggestion,
        const ArrangementAnalysis& analysis) const;
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Set minimum confidence for returned suggestions.
     */
    void setMinConfidence(float confidence);
    
    /**
     * @brief Set maximum suggestions to return.
     */
    void setMaxSuggestions(std::size_t max);
    
private:
    /**
     * @brief Find similar positions in arrangement.
     */
    [[nodiscard]] std::vector<double> findSimilarPositions(
        const ArrangementAnalysis& analysis,
        double beat) const;
    
    float minConfidence_ = 0.3f;
    std::size_t maxSuggestions_ = 10;
};

/**
 * @brief Get string representation of transition type.
 */
[[nodiscard]] const char* toString(TransitionType type);

/**
 * @brief Get string representation of suggestion type.
 */
[[nodiscard]] const char* toString(SuggestionType type);

} // namespace cppmusic::ai::arrangement
