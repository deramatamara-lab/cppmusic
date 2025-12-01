/**
 * @file SuggestionEngine.cpp
 * @brief Implementation of arrangement suggestion generation.
 */

#include "SuggestionEngine.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace cppmusic::ai::arrangement {

const char* toString(TransitionType type) {
    switch (type) {
        case TransitionType::Cut: return "Cut";
        case TransitionType::Fade: return "Fade";
        case TransitionType::Buildup: return "Buildup";
        case TransitionType::Breakdown: return "Breakdown";
        case TransitionType::FilterSweep: return "Filter Sweep";
        case TransitionType::Riser: return "Riser";
    }
    return "Unknown";
}

const char* toString(SuggestionType type) {
    switch (type) {
        case SuggestionType::PatternPlacement: return "Pattern Placement";
        case SuggestionType::Transition: return "Transition";
        case SuggestionType::Fill: return "Fill";
        case SuggestionType::Variation: return "Variation";
        case SuggestionType::Energy: return "Energy";
    }
    return "Unknown";
}

SuggestionEngine::SuggestionEngine() = default;
SuggestionEngine::~SuggestionEngine() = default;

std::vector<Suggestion> SuggestionEngine::getSuggestions(
    const ArrangementAnalysis& analysis,
    double currentBeat,
    const std::vector<std::string>& existingPatterns) const {
    
    std::vector<Suggestion> suggestions;
    
    // Get pattern suggestions
    auto patterns = suggestPatterns(analysis, currentBeat, existingPatterns);
    for (const auto& p : patterns) {
        Suggestion s;
        s.type = SuggestionType::PatternPlacement;
        s.beat = p.suggestedBeat;
        s.confidence = p.confidence;
        s.description = "Place pattern: " + p.patternName;
        s.rationale = p.rationale;
        s.patternSuggestion = p;
        suggestions.push_back(s);
    }
    
    // Get transition suggestions if near section boundary
    const auto* currentSection = analysis.getSectionAt(currentBeat);
    if (currentSection) {
        double sectionEnd = currentSection->endBeat;
        if (sectionEnd - currentBeat < 8.0) {
            auto transitions = suggestTransitions(analysis, currentBeat, sectionEnd + 8.0);
            for (const auto& t : transitions) {
                Suggestion s;
                s.type = SuggestionType::Transition;
                s.beat = t.startBeat;
                s.confidence = t.confidence;
                s.description = std::string("Add ") + toString(t.type) + " transition";
                s.rationale = t.rationale;
                s.transitionSuggestion = t;
                suggestions.push_back(s);
            }
        }
    }
    
    // Get fill suggestions
    auto fills = suggestFills(analysis, currentBeat);
    for (const auto& f : fills) {
        Suggestion s;
        s.type = SuggestionType::Fill;
        s.beat = f.beat;
        s.confidence = f.confidence;
        s.description = f.description;
        s.rationale = f.rationale;
        s.fillSuggestion = f;
        suggestions.push_back(s);
    }
    
    // Filter by confidence
    suggestions.erase(
        std::remove_if(suggestions.begin(), suggestions.end(),
            [this](const Suggestion& s) { return s.confidence < minConfidence_; }),
        suggestions.end());
    
    // Sort by confidence
    std::sort(suggestions.begin(), suggestions.end(),
        [](const Suggestion& a, const Suggestion& b) {
            return a.confidence > b.confidence;
        });
    
    // Limit results
    if (suggestions.size() > maxSuggestions_) {
        suggestions.resize(maxSuggestions_);
    }
    
    return suggestions;
}

std::vector<PatternSuggestion> SuggestionEngine::suggestPatterns(
    const ArrangementAnalysis& analysis,
    double beat,
    const std::vector<std::string>& existingPatterns) const {
    
    std::vector<PatternSuggestion> suggestions;
    
    // Find similar positions in the arrangement
    auto similarPositions = findSimilarPositions(analysis, beat);
    
    // Suggest patterns that work at similar energy levels
    const auto* section = analysis.getSectionAt(beat);
    float energy = analysis.getEnergyAt(beat);
    
    // Generate contextual suggestions
    if (section) {
        PatternSuggestion s;
        s.suggestedBeat = beat;
        s.suggestedTrack = 0;
        
        switch (section->type) {
            case SectionType::Intro:
            case SectionType::Outro:
                s.patternName = "ambient_pad";
                s.confidence = 0.6f;
                s.rationale = "Low energy section benefits from atmospheric elements";
                suggestions.push_back(s);
                break;
                
            case SectionType::Verse:
                s.patternName = "verse_drums";
                s.confidence = 0.7f;
                s.rationale = "Verse section typically uses lighter drum patterns";
                suggestions.push_back(s);
                break;
                
            case SectionType::Chorus:
            case SectionType::Drop:
                s.patternName = "full_drums";
                s.confidence = 0.8f;
                s.rationale = "High energy section benefits from full drum patterns";
                suggestions.push_back(s);
                
                s.patternName = "bass_line";
                s.confidence = 0.7f;
                s.rationale = "Add bass for fuller sound in high energy section";
                suggestions.push_back(s);
                break;
                
            case SectionType::Breakdown:
                s.patternName = "minimal_perc";
                s.confidence = 0.6f;
                s.rationale = "Breakdown benefits from stripped-back percussion";
                suggestions.push_back(s);
                break;
                
            case SectionType::Buildup:
                s.patternName = "riser";
                s.confidence = 0.8f;
                s.rationale = "Buildup section needs tension-building elements";
                suggestions.push_back(s);
                break;
                
            default:
                break;
        }
    }
    
    // Suggest from existing patterns if they match energy
    for (const auto& pattern : existingPatterns) {
        PatternSuggestion s;
        s.patternName = pattern;
        s.suggestedBeat = beat;
        s.suggestedTrack = 0;
        s.confidence = 0.5f;
        s.rationale = "Pattern from your library that may fit here";
        suggestions.push_back(s);
    }
    
    // Suppress unused warning
    (void)energy;
    
    return suggestions;
}

std::vector<TransitionSuggestion> SuggestionEngine::suggestTransitions(
    const ArrangementAnalysis& analysis,
    double fromBeat, double toBeat) const {
    
    std::vector<TransitionSuggestion> suggestions;
    
    float fromEnergy = analysis.getEnergyAt(fromBeat);
    float toEnergy = analysis.getEnergyAt(toBeat);
    float energyChange = toEnergy - fromEnergy;
    
    TransitionSuggestion s;
    s.startBeat = fromBeat;
    s.endBeat = toBeat;
    
    // Suggest based on energy change
    if (energyChange > 0.3f) {
        s.type = TransitionType::Buildup;
        s.confidence = 0.8f;
        s.rationale = "Energy increasing significantly - buildup recommended";
        suggestions.push_back(s);
        
        s.type = TransitionType::Riser;
        s.confidence = 0.6f;
        s.rationale = "Riser can enhance the energy increase";
        suggestions.push_back(s);
    } else if (energyChange < -0.3f) {
        s.type = TransitionType::Breakdown;
        s.confidence = 0.7f;
        s.rationale = "Energy decreasing - breakdown transition fits";
        suggestions.push_back(s);
        
        s.type = TransitionType::FilterSweep;
        s.confidence = 0.5f;
        s.rationale = "Filter sweep can smooth the energy drop";
        suggestions.push_back(s);
    } else {
        s.type = TransitionType::Fade;
        s.confidence = 0.5f;
        s.rationale = "Similar energy levels - smooth crossfade works well";
        suggestions.push_back(s);
        
        s.type = TransitionType::Cut;
        s.confidence = 0.4f;
        s.rationale = "Hard cut for more abrupt change";
        suggestions.push_back(s);
    }
    
    return suggestions;
}

std::vector<FillSuggestion> SuggestionEngine::suggestFills(
    const ArrangementAnalysis& analysis,
    double beat) const {
    
    std::vector<FillSuggestion> suggestions;
    
    // Suggest fills near section boundaries
    for (const auto& section : analysis.sections) {
        double distanceToEnd = section.endBeat - beat;
        
        // Suggest fill 1 bar before section end
        if (distanceToEnd > 0.0 && distanceToEnd <= 4.0) {
            FillSuggestion f;
            f.beat = section.endBeat - 4.0;
            f.duration = 4.0;
            f.confidence = 0.7f;
            f.description = "Add drum fill before section change";
            f.rationale = "Fill helps transition between sections";
            suggestions.push_back(f);
        }
        
        // Suggest fill 2 bars before section end for longer fills
        if (distanceToEnd > 4.0 && distanceToEnd <= 8.0) {
            FillSuggestion f;
            f.beat = section.endBeat - 8.0;
            f.duration = 8.0;
            f.confidence = 0.5f;
            f.description = "Add 2-bar buildup before section change";
            f.rationale = "Longer fill creates more anticipation";
            suggestions.push_back(f);
        }
    }
    
    return suggestions;
}

float SuggestionEngine::scoreSuggestion(
    const Suggestion& suggestion,
    const ArrangementAnalysis& analysis) const {
    
    float score = 0.0f;
    
    // Context fit (40% weight)
    score += 0.4f * computeContextFit(suggestion, analysis);
    
    // Interest contribution (30% weight)
    score += 0.3f * computeInterestContribution(suggestion, analysis);
    
    // Base confidence (20% weight)
    score += 0.2f * suggestion.confidence;
    
    // Novelty (10% weight) - simplified
    score += 0.1f * 0.5f;
    
    return score;
}

float SuggestionEngine::computeContextFit(
    const Suggestion& suggestion,
    const ArrangementAnalysis& analysis) const {
    
    // Check if suggestion fits current section
    const auto* section = analysis.getSectionAt(suggestion.beat);
    if (!section) return 0.5f;
    
    float fit = 0.5f;
    
    // Higher fit for suggestions that match section type
    switch (suggestion.type) {
        case SuggestionType::PatternPlacement:
            if (section->type == SectionType::Chorus || 
                section->type == SectionType::Drop) {
                fit = 0.8f;
            }
            break;
            
        case SuggestionType::Transition:
            // Transitions are always relevant near boundaries
            fit = 0.7f;
            break;
            
        case SuggestionType::Fill:
            // Fills fit well at any section boundary
            fit = 0.6f;
            break;
            
        default:
            break;
    }
    
    return fit;
}

float SuggestionEngine::computeInterestContribution(
    const Suggestion& suggestion,
    const ArrangementAnalysis& analysis) const {
    
    // Estimate how much this suggestion would add to musical interest
    float contribution = 0.5f;
    
    // Higher contribution if arrangement lacks variation
    if (analysis.metrics.melodicVariation < 0.3f) {
        contribution += 0.2f;
    }
    
    // Higher contribution for suggestions that change energy
    if (suggestion.type == SuggestionType::Transition) {
        contribution += 0.1f;
    }
    
    return std::clamp(contribution, 0.0f, 1.0f);
}

std::string SuggestionEngine::generateRationale(
    const Suggestion& suggestion,
    const ArrangementAnalysis& analysis) const {
    
    std::stringstream ss;
    
    const auto* section = analysis.getSectionAt(suggestion.beat);
    if (section) {
        ss << "In " << toString(section->type) << " section. ";
    }
    
    ss << suggestion.rationale;
    
    if (suggestion.confidence > 0.7f) {
        ss << " (high confidence)";
    }
    
    return ss.str();
}

void SuggestionEngine::setMinConfidence(float confidence) {
    minConfidence_ = std::clamp(confidence, 0.0f, 1.0f);
}

void SuggestionEngine::setMaxSuggestions(std::size_t max) {
    maxSuggestions_ = std::max(std::size_t(1), max);
}

std::vector<double> SuggestionEngine::findSimilarPositions(
    const ArrangementAnalysis& analysis,
    double beat) const {
    
    std::vector<double> positions;
    
    // Find positions with similar energy
    float targetEnergy = analysis.getEnergyAt(beat);
    
    for (const auto& section : analysis.sections) {
        double midBeat = (section.startBeat + section.endBeat) / 2.0;
        float sectionEnergy = analysis.getEnergyAt(midBeat);
        
        if (std::abs(sectionEnergy - targetEnergy) < 0.2f) {
            positions.push_back(midBeat);
        }
    }
    
    return positions;
}

} // namespace cppmusic::ai::arrangement
