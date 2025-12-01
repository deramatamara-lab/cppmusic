# AI Arrangement Assistant

This document describes arrangement analysis metrics, suggestion generation, and evaluation scoring.

## Overview

The AI arrangement assistant provides:
- Structural analysis of existing arrangements
- Pattern placement suggestions
- Transition and fill recommendations
- Variation scoring for musical interest

## Architecture

### ArrangementAnalyzer

Analyzes existing arrangement structure:

```cpp
namespace cppmusic::ai::arrangement {

class ArrangementAnalyzer {
public:
    // Analyze full arrangement
    ArrangementAnalysis analyze(const Project& project);
    
    // Analyze specific section
    SectionAnalysis analyzeSection(const Project& project, 
                                   double startBeat, double endBeat);
    
    // Detect structural boundaries
    std::vector<SectionBoundary> detectSections(const Project& project);
    
    // Compute arrangement metrics
    ArrangementMetrics computeMetrics(const Project& project);
};

}
```

### SuggestionEngine

Generates arrangement suggestions:

```cpp
class SuggestionEngine {
public:
    // Get suggestions for current position
    std::vector<Suggestion> getSuggestions(
        const Project& project,
        const ArrangementAnalysis& analysis,
        double currentBeat);
    
    // Specific suggestion types
    std::vector<PatternSuggestion> suggestPatterns(
        const ArrangementAnalysis& analysis, double beat);
    
    std::vector<TransitionSuggestion> suggestTransitions(
        const ArrangementAnalysis& analysis, 
        double fromBeat, double toBeat);
    
    std::vector<FillSuggestion> suggestFills(
        const ArrangementAnalysis& analysis, double beat);
};
```

## Arrangement Analysis Metrics

### Section Detection

Identifies verse, chorus, bridge, etc.:

```cpp
struct SectionBoundary {
    double startBeat;
    double endBeat;
    SectionType type;      // Intro, Verse, Chorus, Bridge, Outro, etc.
    float confidence;
    
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
};
```

### Detection Algorithm

```cpp
std::vector<SectionBoundary> ArrangementAnalyzer::detectSections(
    const Project& project) {
    
    std::vector<SectionBoundary> sections;
    
    // 1. Compute energy curve
    auto energyCurve = computeEnergyCurve(project);
    
    // 2. Detect significant changes
    auto changePoints = detectChangePoints(energyCurve);
    
    // 3. Classify each section
    for (size_t i = 0; i < changePoints.size(); ++i) {
        double start = (i == 0) ? 0 : changePoints[i-1];
        double end = changePoints[i];
        
        SectionBoundary section;
        section.startBeat = start;
        section.endBeat = end;
        section.type = classifySection(project, start, end, energyCurve);
        section.confidence = computeClassificationConfidence();
        
        sections.push_back(section);
    }
    
    return sections;
}
```

### Arrangement Metrics

```cpp
struct ArrangementMetrics {
    // Structure metrics
    float structuralClarity;    // How clear are section boundaries (0-1)
    float formConsistency;      // How consistent is the form (0-1)
    
    // Variation metrics
    float melodicVariation;     // Variation in melodic content (0-1)
    float rhythmicVariation;    // Variation in rhythmic patterns (0-1)
    float timbreVariation;      // Variation in instrumentation (0-1)
    
    // Energy metrics
    float energyRange;          // Dynamic range of energy (0-1)
    float buildupIntensity;     // Strength of buildups (0-1)
    
    // Overall scores
    float interestScore;        // Overall musical interest (0-1)
    float coherenceScore;       // Overall structural coherence (0-1)
};
```

### Metric Computation

```cpp
ArrangementMetrics ArrangementAnalyzer::computeMetrics(const Project& project) {
    ArrangementMetrics metrics;
    
    // Compute variation score
    auto sections = detectSections(project);
    
    float totalVariation = 0.0f;
    for (size_t i = 1; i < sections.size(); ++i) {
        float variation = computeSectionDifference(
            project, sections[i-1], sections[i]);
        totalVariation += variation;
    }
    
    metrics.melodicVariation = totalVariation / (sections.size() - 1);
    
    // ... compute other metrics
    
    return metrics;
}
```

## Suggestion Generation

### Pattern Suggestions

```cpp
struct PatternSuggestion {
    std::string patternName;
    double suggestedBeat;
    int suggestedTrack;
    float confidence;
    std::string rationale;
};

std::vector<PatternSuggestion> SuggestionEngine::suggestPatterns(
    const ArrangementAnalysis& analysis, double beat) {
    
    std::vector<PatternSuggestion> suggestions;
    
    // Find similar positions in existing arrangement
    auto similarPositions = findSimilarPositions(analysis, beat);
    
    // Get patterns used at similar positions
    for (const auto& pos : similarPositions) {
        for (const auto& pattern : getPatternsAt(pos)) {
            PatternSuggestion suggestion;
            suggestion.patternName = pattern.name;
            suggestion.suggestedBeat = beat;
            suggestion.suggestedTrack = pattern.track;
            suggestion.confidence = computePatternConfidence(pattern, beat, analysis);
            suggestion.rationale = generateRationale(pattern, pos, beat);
            
            suggestions.push_back(suggestion);
        }
    }
    
    // Sort by confidence
    std::sort(suggestions.begin(), suggestions.end(),
              [](const auto& a, const auto& b) {
                  return a.confidence > b.confidence;
              });
    
    return suggestions;
}
```

### Transition Suggestions

```cpp
struct TransitionSuggestion {
    TransitionType type;
    double startBeat;
    double endBeat;
    std::vector<AutomationHint> automationHints;
    float confidence;
    std::string rationale;
    
    enum class TransitionType {
        Cut,        // Hard cut
        Fade,       // Crossfade
        Buildup,    // Energy increase
        Breakdown,  // Energy decrease
        FilterSweep,// Filter automation
        Riser       // SFX-based transition
    };
};
```

### Fill Suggestions

```cpp
struct FillSuggestion {
    std::string description;
    double beat;
    double duration;
    std::vector<NoteEvent> notes;
    float confidence;
    std::string rationale;
};
```

## Evaluation Scoring

### Suggestion Quality Score

```cpp
float SuggestionEngine::scoreSuggestion(
    const Suggestion& suggestion,
    const ArrangementAnalysis& analysis) {
    
    float score = 0.0f;
    
    // Context fit (0.4 weight)
    score += 0.4f * computeContextFit(suggestion, analysis);
    
    // Musical interest contribution (0.3 weight)
    score += 0.3f * computeInterestContribution(suggestion, analysis);
    
    // Structural appropriateness (0.2 weight)
    score += 0.2f * computeStructuralFit(suggestion, analysis);
    
    // Novelty (0.1 weight)
    score += 0.1f * computeNovelty(suggestion, analysis);
    
    return score;
}
```

### Context Fit

Measures how well a suggestion fits the current musical context:

```cpp
float SuggestionEngine::computeContextFit(
    const Suggestion& suggestion,
    const ArrangementAnalysis& analysis) {
    
    // Key compatibility
    float keyFit = computeKeyCompatibility(suggestion, analysis.currentKey);
    
    // Energy level match
    float energyFit = computeEnergyMatch(suggestion, analysis.energyAtPosition);
    
    // Instrumentation compatibility
    float instrumentFit = computeInstrumentationFit(suggestion, analysis);
    
    return (keyFit + energyFit + instrumentFit) / 3.0f;
}
```

## Rationale Generation

Explains why a suggestion was made:

```cpp
std::string SuggestionEngine::generateRationale(
    const Suggestion& suggestion,
    const ArrangementAnalysis& analysis) {
    
    std::stringstream rationale;
    
    // Explain pattern basis
    if (suggestion.basedOnPosition >= 0) {
        rationale << "Similar to position " << suggestion.basedOnPosition 
                  << " beats. ";
    }
    
    // Explain musical reasoning
    if (suggestion.type == SuggestionType::Pattern) {
        rationale << "This pattern would add "
                  << describeContribution(suggestion) << ". ";
    }
    
    // Explain structural role
    auto sectionType = analysis.getSectionAt(suggestion.beat);
    rationale << "Appropriate for " << toString(sectionType) << " section.";
    
    return rationale.str();
}
```

## File Layout

```
src/ai/arrangement/
├── ArrangementAnalyzer.hpp/.cpp
├── SuggestionEngine.hpp/.cpp
└── CMakeLists.txt

src/ui/arrangement/
├── SuggestionPanel.cpp
└── CMakeLists.txt
```

## Testing

Key test cases:
- Section detection accuracy on known arrangements
- Variation score is deterministic
- Suggestions are relevant to current context
- Rationale generation produces coherent text
- Scoring is consistent for same inputs
