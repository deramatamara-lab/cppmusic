/**
 * @file ArrangementAnalyzer.cpp
 * @brief Implementation of arrangement structure analysis.
 */

#include "ArrangementAnalyzer.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace cppmusic::ai::arrangement {

const char* toString(SectionType type) {
    switch (type) {
        case SectionType::Intro: return "Intro";
        case SectionType::Verse: return "Verse";
        case SectionType::PreChorus: return "Pre-Chorus";
        case SectionType::Chorus: return "Chorus";
        case SectionType::Bridge: return "Bridge";
        case SectionType::Breakdown: return "Breakdown";
        case SectionType::Buildup: return "Buildup";
        case SectionType::Drop: return "Drop";
        case SectionType::Outro: return "Outro";
        case SectionType::Unknown: return "Unknown";
    }
    return "Unknown";
}

// =============================================================================
// ArrangementAnalysis Implementation
// =============================================================================

const SectionBoundary* ArrangementAnalysis::getSectionAt(double beat) const {
    for (const auto& section : sections) {
        if (beat >= section.startBeat && beat < section.endBeat) {
            return &section;
        }
    }
    return nullptr;
}

float ArrangementAnalysis::getEnergyAt(double beat) const {
    if (energyCurve.empty()) return 0.5f;
    
    // Find surrounding energy points
    auto it = std::lower_bound(energyCurve.begin(), energyCurve.end(), beat,
        [](const EnergyPoint& p, double b) { return p.beat < b; });
    
    if (it == energyCurve.begin()) {
        return energyCurve.front().energy;
    }
    if (it == energyCurve.end()) {
        return energyCurve.back().energy;
    }
    
    // Linear interpolation
    const auto& next = *it;
    const auto& prev = *std::prev(it);
    
    double range = next.beat - prev.beat;
    if (range <= 0.0) return prev.energy;
    
    float t = static_cast<float>((beat - prev.beat) / range);
    return prev.energy + t * (next.energy - prev.energy);
}

// =============================================================================
// ArrangementAnalyzer Implementation
// =============================================================================

ArrangementAnalyzer::ArrangementAnalyzer() = default;
ArrangementAnalyzer::~ArrangementAnalyzer() = default;

std::vector<EnergyPoint> ArrangementAnalyzer::analyzeEnergy(
    const std::vector<double>& beats,
    const std::vector<float>& velocities,
    double totalBeats) const {
    
    std::vector<EnergyPoint> curve;
    
    if (beats.empty() || totalBeats <= 0.0) {
        return curve;
    }
    
    // Create energy curve at regular intervals
    double step = static_cast<double>(energyWindowSize_);
    
    for (double beat = 0.0; beat < totalBeats; beat += step) {
        double windowEnd = beat + step;
        
        // Count notes and sum velocities in window
        float energy = 0.0f;
        int noteCount = 0;
        
        for (std::size_t i = 0; i < beats.size(); ++i) {
            if (beats[i] >= beat && beats[i] < windowEnd) {
                float vel = (i < velocities.size()) ? velocities[i] : 0.75f;
                energy += vel;
                ++noteCount;
            }
        }
        
        // Normalize energy
        if (noteCount > 0) {
            energy = energy / static_cast<float>(noteCount);
            energy *= std::min(1.0f, static_cast<float>(noteCount) / 8.0f);
        }
        
        curve.push_back({beat, energy});
    }
    
    // Normalize curve to 0-1 range
    if (!curve.empty()) {
        float maxEnergy = 0.0f;
        for (const auto& p : curve) {
            maxEnergy = std::max(maxEnergy, p.energy);
        }
        
        if (maxEnergy > 0.0f) {
            for (auto& p : curve) {
                p.energy /= maxEnergy;
            }
        }
    }
    
    return curve;
}

std::vector<SectionBoundary> ArrangementAnalyzer::detectSections(
    const std::vector<EnergyPoint>& energyCurve) const {
    
    std::vector<SectionBoundary> sections;
    
    if (energyCurve.size() < 2) {
        return sections;
    }
    
    // Find change points where energy changes significantly
    std::vector<double> changePoints;
    changePoints.push_back(energyCurve.front().beat);
    
    for (std::size_t i = 1; i < energyCurve.size(); ++i) {
        float change = std::abs(energyCurve[i].energy - energyCurve[i - 1].energy);
        
        if (change > energyChangeThreshold_) {
            // Check minimum distance from last change point
            if (changePoints.empty() || 
                (energyCurve[i].beat - changePoints.back()) >= minSectionLength_) {
                changePoints.push_back(energyCurve[i].beat);
            }
        }
    }
    
    // Add final point
    if (changePoints.back() < energyCurve.back().beat) {
        changePoints.push_back(energyCurve.back().beat);
    }
    
    // Create sections from change points
    for (std::size_t i = 0; i + 1 < changePoints.size(); ++i) {
        SectionBoundary section;
        section.startBeat = changePoints[i];
        section.endBeat = changePoints[i + 1];
        section.type = classifySection(energyCurve, section.startBeat, section.endBeat,
                                        i, changePoints.size() - 1);
        section.confidence = 0.7f;  // Placeholder confidence
        
        sections.push_back(section);
    }
    
    return sections;
}

SectionType ArrangementAnalyzer::classifySection(
    const std::vector<EnergyPoint>& energyCurve,
    double startBeat, double endBeat,
    std::size_t sectionIndex, std::size_t totalSections) const {
    
    // Calculate average energy in section
    float avgEnergy = 0.0f;
    int count = 0;
    
    for (const auto& p : energyCurve) {
        if (p.beat >= startBeat && p.beat < endBeat) {
            avgEnergy += p.energy;
            ++count;
        }
    }
    
    if (count > 0) {
        avgEnergy /= static_cast<float>(count);
    }
    
    // Calculate energy trend (rising, falling, stable)
    float startEnergy = 0.0f;
    float endEnergy = 0.0f;
    
    for (const auto& p : energyCurve) {
        if (p.beat >= startBeat && p.beat < startBeat + 4.0) {
            startEnergy = p.energy;
        }
        if (p.beat <= endBeat && p.beat > endBeat - 4.0) {
            endEnergy = p.energy;
        }
    }
    
    float trend = endEnergy - startEnergy;
    
    // Classify based on position, energy, and trend
    float relativePosition = static_cast<float>(sectionIndex) / 
                            static_cast<float>(std::max(std::size_t(1), totalSections));
    
    // Intro: at start, low energy
    if (sectionIndex == 0 && avgEnergy < 0.4f) {
        return SectionType::Intro;
    }
    
    // Outro: at end, energy may be decreasing
    if (sectionIndex == totalSections - 1 && (trend < -0.1f || avgEnergy < 0.3f)) {
        return SectionType::Outro;
    }
    
    // Buildup: rising energy
    if (trend > 0.2f) {
        return SectionType::Buildup;
    }
    
    // Drop: high energy following buildup
    if (avgEnergy > 0.7f && relativePosition > 0.3f) {
        return SectionType::Drop;
    }
    
    // Breakdown: low energy in middle
    if (avgEnergy < 0.3f && relativePosition > 0.2f && relativePosition < 0.8f) {
        return SectionType::Breakdown;
    }
    
    // Chorus: high energy, stable
    if (avgEnergy > 0.6f && std::abs(trend) < 0.1f) {
        return SectionType::Chorus;
    }
    
    // Verse: medium energy
    if (avgEnergy > 0.3f && avgEnergy < 0.6f) {
        return SectionType::Verse;
    }
    
    return SectionType::Unknown;
}

ArrangementMetrics ArrangementAnalyzer::computeMetrics(
    const std::vector<SectionBoundary>& sections,
    const std::vector<EnergyPoint>& energyCurve) const {
    
    ArrangementMetrics metrics;
    
    if (sections.empty() || energyCurve.empty()) {
        return metrics;
    }
    
    // Structural clarity: based on section count and regularity
    float avgSectionLength = 0.0f;
    for (const auto& s : sections) {
        avgSectionLength += static_cast<float>(s.getDuration());
    }
    avgSectionLength /= static_cast<float>(sections.size());
    
    float lengthVariance = 0.0f;
    for (const auto& s : sections) {
        float diff = static_cast<float>(s.getDuration()) - avgSectionLength;
        lengthVariance += diff * diff;
    }
    lengthVariance /= static_cast<float>(sections.size());
    
    metrics.structuralClarity = 1.0f / (1.0f + lengthVariance / 100.0f);
    
    // Form consistency: check for repeating section patterns
    std::vector<SectionType> pattern;
    for (const auto& s : sections) {
        pattern.push_back(s.type);
    }
    
    // Simple check: count unique section types
    std::vector<SectionType> uniqueTypes = pattern;
    std::sort(uniqueTypes.begin(), uniqueTypes.end());
    uniqueTypes.erase(std::unique(uniqueTypes.begin(), uniqueTypes.end()), uniqueTypes.end());
    
    metrics.formConsistency = 1.0f - (static_cast<float>(uniqueTypes.size()) / 
                                      static_cast<float>(sections.size()));
    
    // Energy range
    float minEnergy = 1.0f;
    float maxEnergy = 0.0f;
    for (const auto& p : energyCurve) {
        minEnergy = std::min(minEnergy, p.energy);
        maxEnergy = std::max(maxEnergy, p.energy);
    }
    metrics.energyRange = maxEnergy - minEnergy;
    
    // Variation metrics
    if (sections.size() > 1) {
        float totalVariation = 0.0f;
        for (std::size_t i = 1; i < sections.size(); ++i) {
            totalVariation += computeSectionDifference(energyCurve, 
                                                       sections[i - 1], sections[i]);
        }
        metrics.melodicVariation = totalVariation / static_cast<float>(sections.size() - 1);
        metrics.rhythmicVariation = metrics.melodicVariation;  // Simplified
    }
    
    // Overall scores
    metrics.interestScore = (metrics.energyRange + metrics.melodicVariation) / 2.0f;
    metrics.coherenceScore = (metrics.structuralClarity + metrics.formConsistency) / 2.0f;
    
    return metrics;
}

float ArrangementAnalyzer::computeSectionDifference(
    const std::vector<EnergyPoint>& energyCurve,
    const SectionBoundary& section1,
    const SectionBoundary& section2) const {
    
    // Compute average energy difference
    float avg1 = 0.0f;
    float avg2 = 0.0f;
    int count1 = 0;
    int count2 = 0;
    
    for (const auto& p : energyCurve) {
        if (p.beat >= section1.startBeat && p.beat < section1.endBeat) {
            avg1 += p.energy;
            ++count1;
        }
        if (p.beat >= section2.startBeat && p.beat < section2.endBeat) {
            avg2 += p.energy;
            ++count2;
        }
    }
    
    if (count1 > 0) avg1 /= static_cast<float>(count1);
    if (count2 > 0) avg2 /= static_cast<float>(count2);
    
    return std::abs(avg1 - avg2);
}

void ArrangementAnalyzer::setMinSectionLength(double beats) {
    minSectionLength_ = std::max(4.0, beats);
}

void ArrangementAnalyzer::setEnergyChangeThreshold(float threshold) {
    energyChangeThreshold_ = std::clamp(threshold, 0.05f, 0.5f);
}

} // namespace cppmusic::ai::arrangement
