#include "ChordAdvisor.h"
#include "../inference/InferenceEngine.h"
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace daw::ai::models
{

namespace
{
    // Chord definitions (intervals from root in semitones)
    struct ChordType
    {
        std::string name;
        std::vector<int> intervals;
        float commonality; // How common this chord is (0-1)
    };

    const std::vector<ChordType> chordTypes = {
        {"maj", {0, 4, 7}, 1.0f},
        {"min", {0, 3, 7}, 0.9f},
        {"maj7", {0, 4, 7, 11}, 0.8f},
        {"min7", {0, 3, 7, 10}, 0.8f},
        {"dom7", {0, 4, 7, 10}, 0.7f},
        {"dim", {0, 3, 6}, 0.4f},
        {"aug", {0, 4, 8}, 0.3f},
        {"sus2", {0, 2, 7}, 0.5f},
        {"sus4", {0, 5, 7}, 0.5f},
        {"add9", {0, 4, 7, 14}, 0.6f}
    };

    constexpr std::array<const char*, 12> noteNames = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    std::string getNoteName(int midiNote)
    {
        return std::string(noteNames[midiNote % 12]);
    }

    // Analyze notes to detect chord
    struct ChordMatch
    {
        int root;
        const ChordType* type;
        float confidence;
        std::set<int> matchedNotes;
    };

    std::vector<ChordMatch> analyzeChords(const std::vector<int>& notes)
    {
        if (notes.empty())
            return {};

        std::vector<ChordMatch> matches;

        // Try each possible root note
        for (int root = 0; root < 12; ++root)
        {
            // Try each chord type
            for (const auto& chordType : chordTypes)
            {
                ChordMatch match;
                match.root = root;
                match.type = &chordType;
                match.confidence = 0.0f;

                // Check how many notes match this chord
                std::set<int> noteClasses;
                for (int note : notes)
                {
                    noteClasses.insert(note % 12);
                }

                int matchedCount = 0;
                for (int interval : chordType.intervals)
                {
                    const int noteClass = (root + interval) % 12;
                    if (noteClasses.count(noteClass) > 0)
                    {
                        ++matchedCount;
                        match.matchedNotes.insert(noteClass);
                    }
                }

                // Calculate confidence based on match ratio and commonality
                if (matchedCount > 0)
                {
                    const float matchRatio = static_cast<float>(matchedCount) / static_cast<float>(chordType.intervals.size());
                    match.confidence = matchRatio * chordType.commonality;
                    matches.push_back(match);
                }
            }
        }

        // Sort by confidence
        std::sort(matches.begin(), matches.end(),
                  [](const ChordMatch& a, const ChordMatch& b) {
                      return a.confidence > b.confidence;
                  });

        return matches;
    }

    // Generate voice-leading suggestions
    std::vector<int> getVoiceLeadingChord(int currentRoot, int targetRoot, const ChordType& type)
    {
        std::vector<int> notes;
        const int baseOctave = 60; // Middle C octave

        for (int interval : type.intervals)
        {
            const int noteClass = (targetRoot + interval) % 12;

            // Find nearest note to current position (voice leading)
            int bestNote = baseOctave + noteClass;

            // Prefer notes in similar register
            if (std::abs(bestNote - (baseOctave + currentRoot)) > 6)
            {
                bestNote += (bestNote < baseOctave + currentRoot) ? 12 : -12;
            }

            notes.push_back(bestNote);
        }

        return notes;
    }

    // Common chord progressions
    const std::vector<std::vector<int>> progressions = {
        {0, 4, 5, 0},      // I-V-vi-IV (pop progression)
        {0, 5, 3, 4},      // I-vi-IV-V
        {0, 3, 4, 0},      // i-iv-V-i (minor)
        {0, 5, 3, 0},      // I-vi-iii-I
        {0, 2, 5, 0}       // I-ii-V-I (jazz)
    };
}

ChordAdvisor::ChordAdvisor(std::shared_ptr<daw::ai::inference::InferenceEngine> engine)
    : inferenceEngine(engine)
{
}

void ChordAdvisor::getSuggestions(const std::vector<int>& currentNotes, const std::string& key,
                                   std::function<void(std::vector<ChordSuggestion>)> callback)
{
    // Real chord analysis using music theory
    // Detects current chord and suggests voice-leading progressions

    if (currentNotes.empty())
    {
        // Return default suggestions
        std::vector<ChordSuggestion> suggestions;

        ChordSuggestion suggestion;
        suggestion.name = "Cmaj";
        suggestion.notes = {60, 64, 67};
        suggestion.confidence = 0.8f;
        suggestions.push_back(suggestion);

        callback(suggestions);
        return;
    }

    // Convert to input data
    std::vector<float> inputData;
    inputData.reserve(currentNotes.size() + 1);

    for (int note : currentNotes)
    {
        inputData.push_back(static_cast<float>(note) / 127.0f);
    }

    // Add key information
    int keyRoot = 0; // C default
    if (!key.empty())
    {
        for (size_t i = 0; i < 12; ++i)
        {
            if (key.find(noteNames[i]) != std::string::npos)
            {
                keyRoot = static_cast<int>(i);
                break;
            }
        }
    }
    inputData.push_back(static_cast<float>(keyRoot) / 12.0f);

    daw::ai::inference::InferenceRequest request;
    request.inputData = inputData;
    request.callback = [callback, currentNotes, keyRoot](std::vector<float> output)
    {
        std::vector<ChordSuggestion> suggestions;

        // Analyze current chord
        std::vector<int> noteClasses;
        for (int note : currentNotes)
        {
            noteClasses.push_back(note % 12);
        }

        const auto matches = analyzeChords(noteClasses);

        if (matches.empty())
        {
            // Fallback: simple major chord
            ChordSuggestion suggestion;
            suggestion.name = getNoteName(keyRoot) + "maj";
            suggestion.notes = {keyRoot + 60, keyRoot + 64, keyRoot + 67};
            suggestion.confidence = 0.5f;
            suggestions.push_back(suggestion);
            callback(suggestions);
            return;
        }

        // Current chord info
        const int currentRoot = matches[0].root;
        const ChordType* currentType = matches[0].type;

        // Generate suggestions based on progressions
        size_t progressionIndex = 0;
        if (!output.empty())
        {
            progressionIndex = static_cast<size_t>(juce::jlimit(0, static_cast<int>(progressions.size() - 1),
                                                              static_cast<int>(output[0] * progressions.size())));
        }

        const std::vector<int>& progression = progressions[progressionIndex];

        // Generate 3-4 suggestions following the progression
        for (size_t i = 0; i < std::min(progression.size(), size_t(4)); ++i)
        {
            const int progressionStep = progression[i];
            const int targetRoot = (currentRoot + progressionStep) % 12;

            // Use same chord type or common variant
            const ChordType* targetType = currentType;
            if (i > 0 && progressionStep == 4) // Dominant
            {
                // Find dom7 type
                for (const auto& type : chordTypes)
                {
                    if (type.name == "dom7")
                    {
                        targetType = &type;
                        break;
                    }
                }
            }

            ChordSuggestion suggestion;
            suggestion.name = getNoteName(targetRoot) + targetType->name;
            suggestion.notes = getVoiceLeadingChord(currentRoot, targetRoot, *targetType);

            // Confidence based on progression position and chord commonality
            suggestion.confidence = targetType->commonality * (1.0f - static_cast<float>(i) * 0.15f);
            suggestion.confidence = juce::jlimit(0.3f, 1.0f, suggestion.confidence);

            suggestions.push_back(suggestion);
        }

        // Sort by confidence
        std::sort(suggestions.begin(), suggestions.end(),
                  [](const ChordSuggestion& a, const ChordSuggestion& b) {
                      return a.confidence > b.confidence;
                  });

        callback(suggestions);
    };

    inferenceEngine->queueInference(request);
}

} // namespace daw::ai::models
