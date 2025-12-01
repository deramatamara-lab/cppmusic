/**
 * @file HarmonicAnalyzer.cpp
 * @brief Implementation of harmonic analysis.
 */

#include "HarmonicAnalyzer.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace cppmusic::ai::harmony {

namespace {

const char* kPitchClassNames[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

const char* kIntervalNames[] = {
    "P1", "m2", "M2", "m3", "M3", "P4", "TT", "P5", "m6", "M6", "m7", "M7"
};

// Chord templates (pitch class bitmasks for root = 0)
struct ChordTemplate {
    ChordQuality quality;
    std::array<bool, 12> mask;
};

// NOLINTBEGIN(readability-magic-numbers)
const ChordTemplate kChordTemplates[] = {
    {ChordQuality::Major,          {1,0,0,0,1,0,0,1,0,0,0,0}},  // C E G
    {ChordQuality::Minor,          {1,0,0,1,0,0,0,1,0,0,0,0}},  // C Eb G
    {ChordQuality::Diminished,     {1,0,0,1,0,0,1,0,0,0,0,0}},  // C Eb Gb
    {ChordQuality::Augmented,      {1,0,0,0,1,0,0,0,1,0,0,0}},  // C E G#
    {ChordQuality::Dominant7,      {1,0,0,0,1,0,0,1,0,0,1,0}},  // C E G Bb
    {ChordQuality::Major7,         {1,0,0,0,1,0,0,1,0,0,0,1}},  // C E G B
    {ChordQuality::Minor7,         {1,0,0,1,0,0,0,1,0,0,1,0}},  // C Eb G Bb
    {ChordQuality::Diminished7,    {1,0,0,1,0,0,1,0,0,1,0,0}},  // C Eb Gb Bbb
    {ChordQuality::HalfDiminished7,{1,0,0,1,0,0,1,0,0,0,1,0}},  // C Eb Gb Bb
    {ChordQuality::Sus2,           {1,0,1,0,0,0,0,1,0,0,0,0}},  // C D G
    {ChordQuality::Sus4,           {1,0,0,0,0,1,0,1,0,0,0,0}},  // C F G
};
// NOLINTEND(readability-magic-numbers)

// Major key profile (Krumhansl-Kessler)
const std::array<float, 12> kMajorProfile = {
    6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f,
    2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f
};

// Minor key profile
const std::array<float, 12> kMinorProfile = {
    6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f,
    2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f
};

float correlate(const std::array<float, 12>& a, const std::array<float, 12>& b) {
    float sumA = 0.0f;
    float sumB = 0.0f;
    for (int i = 0; i < 12; ++i) {
        sumA += a[static_cast<std::size_t>(i)];
        sumB += b[static_cast<std::size_t>(i)];
    }
    float meanA = sumA / 12.0f;
    float meanB = sumB / 12.0f;
    
    float num = 0.0f, denomA = 0.0f, denomB = 0.0f;
    for (int i = 0; i < 12; ++i) {
        float diffA = a[static_cast<std::size_t>(i)] - meanA;
        float diffB = b[static_cast<std::size_t>(i)] - meanB;
        num += diffA * diffB;
        denomA += diffA * diffA;
        denomB += diffB * diffB;
    }
    
    float denom = std::sqrt(denomA * denomB);
    if (denom < 1e-6f) return 0.0f;
    return num / denom;
}

std::array<float, 12> rotateProfile(const std::array<float, 12>& profile, int shift) {
    std::array<float, 12> result{};
    for (int i = 0; i < 12; ++i) {
        result[static_cast<std::size_t>(i)] = profile[static_cast<std::size_t>((i + shift) % 12)];
    }
    return result;
}

} // anonymous namespace

// =============================================================================
// PitchClassVector Implementation
// =============================================================================

void PitchClassVector::normalize() {
    float sum = std::accumulate(values.begin(), values.end(), 0.0f);
    if (sum > 1e-6f) {
        for (auto& v : values) {
            v /= sum;
        }
    }
}

std::vector<int> PitchClassVector::getDominant(int count) const {
    std::vector<std::pair<float, int>> sorted;
    for (int i = 0; i < 12; ++i) {
        sorted.push_back({values[static_cast<std::size_t>(i)], i});
    }
    
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    std::vector<int> result;
    for (int i = 0; i < std::min(count, 12); ++i) {
        if (sorted[static_cast<std::size_t>(i)].first > 0.0f) {
            result.push_back(sorted[static_cast<std::size_t>(i)].second);
        }
    }
    return result;
}

void PitchClassVector::clear() {
    values.fill(0.0f);
}

void PitchClassVector::addPitch(int midiNote, float weight) {
    int pc = midiNote % 12;
    values[static_cast<std::size_t>(pc)] += weight;
}

// =============================================================================
// ChordInfo Implementation
// =============================================================================

std::string ChordInfo::getName() const {
    std::string name = kPitchClassNames[rootPitchClass];
    
    switch (quality) {
        case ChordQuality::Major: break;  // Just root name
        case ChordQuality::Minor: name += "m"; break;
        case ChordQuality::Diminished: name += "dim"; break;
        case ChordQuality::Augmented: name += "aug"; break;
        case ChordQuality::Dominant7: name += "7"; break;
        case ChordQuality::Major7: name += "maj7"; break;
        case ChordQuality::Minor7: name += "m7"; break;
        case ChordQuality::Diminished7: name += "dim7"; break;
        case ChordQuality::HalfDiminished7: name += "m7b5"; break;
        case ChordQuality::Sus2: name += "sus2"; break;
        case ChordQuality::Sus4: name += "sus4"; break;
        case ChordQuality::Unknown: name += "?"; break;
    }
    
    return name;
}

std::string ChordInfo::getRomanNumeral(int keyRoot, bool isMinor) const {
    int degree = (rootPitchClass - keyRoot + 12) % 12;
    
    // Map semitones to scale degrees (simplified)
    const char* majorNumerals[] = {"I", "bII", "II", "bIII", "III", "IV", "#IV", "V", "bVI", "VI", "bVII", "VII"};
    const char* minorNumerals[] = {"i", "bII", "ii", "III", "iv", "v", "VI", "VII", "bVI", "vi", "VII", "vii"};
    
    std::string numeral = isMinor ? minorNumerals[degree] : majorNumerals[degree];
    
    // Add quality suffix if different from expected
    bool expectMinor = isMinor ? (degree == 0 || degree == 5 || degree == 7) 
                               : (degree == 2 || degree == 4 || degree == 9);
    
    if (quality == ChordQuality::Diminished) {
        numeral += "°";
    } else if (quality == ChordQuality::Augmented) {
        numeral += "+";
    } else if (quality == ChordQuality::Dominant7) {
        numeral += "7";
    } else if (quality == ChordQuality::Major7) {
        numeral += "Δ7";
    } else if (quality == ChordQuality::Minor7) {
        numeral += "-7";
    }
    
    (void)expectMinor;  // Suppress unused warning
    
    return numeral;
}

// =============================================================================
// KeyInfo Implementation
// =============================================================================

std::string KeyInfo::getName() const {
    std::string name = kPitchClassNames[rootPitchClass];
    name += isMinor ? " minor" : " major";
    return name;
}

// =============================================================================
// HarmonicAnalyzer Implementation
// =============================================================================

HarmonicAnalyzer::HarmonicAnalyzer() = default;
HarmonicAnalyzer::~HarmonicAnalyzer() = default;

PitchClassVector HarmonicAnalyzer::createPCV(
    const std::vector<std::uint8_t>& notes,
    const std::vector<std::uint8_t>& velocities) const {
    
    PitchClassVector pcv;
    
    for (std::size_t i = 0; i < notes.size(); ++i) {
        float weight = 1.0f;
        if (i < velocities.size() && velocities[i] > 0) {
            weight = static_cast<float>(velocities[i]) / 127.0f;
        }
        pcv.addPitch(notes[i], weight);
    }
    
    pcv.normalize();
    return pcv;
}

float HarmonicAnalyzer::computeTension(const PitchClassVector& pcv) const {
    float tension = 0.0f;
    
    // Count interval types
    int minorSecondCount = 0;
    int tritoneCount = 0;
    int perfectFifthCount = 0;
    int activeCount = 0;
    
    for (int i = 0; i < 12; ++i) {
        if (pcv.values[static_cast<std::size_t>(i)] > 0.1f) {
            ++activeCount;
            
            for (int j = i + 1; j < 12; ++j) {
                if (pcv.values[static_cast<std::size_t>(j)] > 0.1f) {
                    int interval = j - i;
                    
                    if (interval == 1 || interval == 11) {
                        ++minorSecondCount;  // m2 or M7
                    } else if (interval == 6) {
                        ++tritoneCount;
                    } else if (interval == 7 || interval == 5) {
                        ++perfectFifthCount;  // P5 or P4
                    }
                }
            }
        }
    }
    
    // Compute weighted tension
    tension += weightMinorSecond_ * static_cast<float>(minorSecondCount);
    tension += weightTritone_ * static_cast<float>(tritoneCount);
    tension -= weightPerfectFifth_ * static_cast<float>(perfectFifthCount);
    tension += weightDensity_ * static_cast<float>(std::max(0, activeCount - 3));
    
    return std::clamp(tension, 0.0f, 1.0f);
}

float HarmonicAnalyzer::computeTransitionTension(
    const PitchClassVector& current,
    const PitchClassVector& next) const {
    
    // Measure difference between pitch class vectors
    float diff = 0.0f;
    for (int i = 0; i < 12; ++i) {
        diff += std::abs(current.values[static_cast<std::size_t>(i)] - 
                        next.values[static_cast<std::size_t>(i)]);
    }
    
    // Combine with individual tensions
    float currentTension = computeTension(current);
    float nextTension = computeTension(next);
    
    return std::clamp((diff + currentTension + nextTension) / 3.0f, 0.0f, 1.0f);
}

ChordInfo HarmonicAnalyzer::detectChord(const PitchClassVector& pcv) const {
    ChordInfo bestChord;
    float bestScore = 0.0f;
    
    // Try each possible root
    for (int root = 0; root < 12; ++root) {
        // Try each chord template
        for (const auto& templ : kChordTemplates) {
            float score = 0.0f;
            float totalWeight = 0.0f;
            
            for (int i = 0; i < 12; ++i) {
                int rotatedIdx = (i + root) % 12;
                float weight = pcv.values[static_cast<std::size_t>(rotatedIdx)];
                
                if (templ.mask[static_cast<std::size_t>(i)]) {
                    score += weight;  // Reward matching pitch classes
                } else {
                    score -= weight * 0.5f;  // Penalize non-matching
                }
                totalWeight += weight;
            }
            
            if (totalWeight > 0.0f) {
                score /= totalWeight;
            }
            
            if (score > bestScore) {
                bestScore = score;
                bestChord.rootPitchClass = root;
                bestChord.quality = templ.quality;
            }
        }
    }
    
    bestChord.confidence = std::clamp(bestScore, 0.0f, 1.0f);
    return bestChord;
}

ChordInfo HarmonicAnalyzer::detectChord(const std::vector<std::uint8_t>& notes) const {
    return detectChord(createPCV(notes));
}

KeyInfo HarmonicAnalyzer::detectKey(const PitchClassVector& pcv) const {
    KeyInfo bestKey;
    float bestCorr = -2.0f;
    
    // Try each possible key (major and minor)
    for (int root = 0; root < 12; ++root) {
        // Major
        auto majorProfile = rotateProfile(kMajorProfile, 12 - root);
        float majorCorr = correlate(pcv.values, majorProfile);
        
        if (majorCorr > bestCorr) {
            bestCorr = majorCorr;
            bestKey.rootPitchClass = root;
            bestKey.isMinor = false;
        }
        
        // Minor
        auto minorProfile = rotateProfile(kMinorProfile, 12 - root);
        float minorCorr = correlate(pcv.values, minorProfile);
        
        if (minorCorr > bestCorr) {
            bestCorr = minorCorr;
            bestKey.rootPitchClass = root;
            bestKey.isMinor = true;
        }
    }
    
    bestKey.confidence = std::clamp((bestCorr + 1.0f) / 2.0f, 0.0f, 1.0f);
    return bestKey;
}

KeyInfo HarmonicAnalyzer::detectKeyFromChords(const std::vector<ChordInfo>& chords) const {
    // Aggregate pitch classes from chords
    PitchClassVector aggregated;
    
    for (const auto& chord : chords) {
        // Add chord root with high weight
        aggregated.values[static_cast<std::size_t>(chord.rootPitchClass)] += 2.0f;
        
        // Add expected chord tones based on quality
        int third = (chord.quality == ChordQuality::Minor || 
                     chord.quality == ChordQuality::Minor7 ||
                     chord.quality == ChordQuality::Diminished ||
                     chord.quality == ChordQuality::Diminished7) ? 3 : 4;
        int fifth = (chord.quality == ChordQuality::Diminished ||
                     chord.quality == ChordQuality::Diminished7) ? 6 : 7;
        
        aggregated.values[static_cast<std::size_t>((chord.rootPitchClass + third) % 12)] += 1.0f;
        aggregated.values[static_cast<std::size_t>((chord.rootPitchClass + fifth) % 12)] += 1.0f;
    }
    
    aggregated.normalize();
    return detectKey(aggregated);
}

void HarmonicAnalyzer::setTensionWeights(float minorSecond, float tritone,
                                          float perfectFifth, float density) {
    weightMinorSecond_ = minorSecond;
    weightTritone_ = tritone;
    weightPerfectFifth_ = perfectFifth;
    weightDensity_ = density;
}

const char* getPitchClassName(int pitchClass) {
    return kPitchClassNames[pitchClass % 12];
}

const char* getIntervalName(int semitones) {
    return kIntervalNames[semitones % 12];
}

} // namespace cppmusic::ai::harmony
