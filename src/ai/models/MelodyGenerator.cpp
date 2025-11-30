#include "MelodyGenerator.h"
#include "../inference/InferenceEngine.h"
#include <algorithm>
#include <cmath>
#include <map>

namespace daw::ai::models
{

namespace
{
    // Scale definitions (MIDI note offsets from root)
    const std::map<std::string, std::vector<int>> scales = {
        {"major", {0, 2, 4, 5, 7, 9, 11}},
        {"minor", {0, 2, 3, 5, 7, 8, 10}},
        {"pentatonic", {0, 2, 4, 7, 9}},
        {"blues", {0, 3, 5, 6, 7, 10}},
        {"dorian", {0, 2, 3, 5, 7, 9, 10}},
        {"mixolydian", {0, 2, 4, 5, 7, 9, 10}}
    };

    // Common interval patterns for melody generation
    const std::vector<std::vector<int>> melodicPatterns = {
        {0, 2, 4, 2, 0},           // Ascending/descending
        {0, 4, 7, 4, 0},           // Triadic
        {0, 2, 0, -2, 0},          // Neighbor tones
        {0, 7, 5, 4, 2},           // Stepwise descent
        {0, 4, 2, 5, 7}            // Mixed intervals
    };

    int detectKey(const std::vector<daw::project::Pattern::MIDINote>& notes)
    {
        if (notes.empty())
            return 60; // Default to C

        // Simple key detection: find most common note class
        std::vector<int> noteClasses(12, 0);
        for (const auto& note : notes)
        {
            ++noteClasses[note.note % 12];
        }

        const auto maxIt = std::max_element(noteClasses.begin(), noteClasses.end());
        const int rootClass = static_cast<int>(std::distance(noteClasses.begin(), maxIt));
        return rootClass + 60; // Return MIDI note in middle octave
    }

    std::string detectStyle(const std::vector<daw::project::Pattern::MIDINote>& notes)
    {
        if (notes.empty())
            return "major";

        // Analyze intervals to determine scale
        std::vector<int> intervals;
        for (size_t i = 1; i < notes.size(); ++i)
        {
            intervals.push_back((notes[i].note - notes[i-1].note + 12) % 12);
        }

        // Check for minor third (characteristic of minor scales)
        bool hasMinorThird = false;
        for (int interval : intervals)
        {
            if (interval == 3)
            {
                hasMinorThird = true;
                break;
            }
        }

        return hasMinorThird ? "minor" : "major";
    }
}

MelodyGenerator::MelodyGenerator(std::shared_ptr<daw::ai::inference::InferenceEngine> engine)
    : inferenceEngine(engine)
{
}

void MelodyGenerator::generateMelody(const std::vector<daw::project::Pattern::MIDINote>& contextNotes,
                                      const std::string& style, double lengthBeats,
                                      std::function<void(MelodyResult)> callback)
{
    // Real melody generation using music theory
    // Uses scale-based generation with interval patterns and voice leading

    if (lengthBeats <= 0.0)
    {
        MelodyResult result;
        result.success = false;
        callback(result);
        return;
    }

    // Detect key and scale from context
    const int rootNote = detectKey(contextNotes);
    const std::string detectedStyle = style.empty() ? detectStyle(contextNotes) : style;

    // Get scale intervals
    auto scaleIt = scales.find(detectedStyle);
    if (scaleIt == scales.end())
        scaleIt = scales.find("major"); // Fallback

    const std::vector<int>& scaleIntervals = scaleIt->second;
    const int rootClass = rootNote % 12;

    // Convert context to input for processing
    std::vector<float> inputData;
    inputData.reserve(contextNotes.size() * 3 + 3);

    for (const auto& note : contextNotes)
    {
        inputData.push_back(static_cast<float>(note.note) / 127.0f);
        inputData.push_back(static_cast<float>(note.velocity) / 127.0f);
        inputData.push_back(static_cast<float>(note.startBeat));
    }

    // Add metadata
    inputData.push_back(static_cast<float>(rootNote) / 127.0f);
    inputData.push_back(static_cast<float>(lengthBeats) / 32.0f); // Normalize to reasonable range
    inputData.push_back(static_cast<float>(scaleIntervals.size()) / 12.0f);

    daw::ai::inference::InferenceRequest request;
    request.inputData = inputData;
    request.callback = [callback, rootNote, rootClass, scaleIntervals, lengthBeats, detectedStyle, contextNotes](std::vector<float> output)
    {
        MelodyResult result;
        result.success = true;
        result.style = detectedStyle;

        // Generate melody notes using scale and patterns
        const double noteDuration = 0.25; // Quarter note default
        const int numNotes = static_cast<int>(std::ceil(lengthBeats / noteDuration));

        // Start from last context note or root
        int currentNote = contextNotes.empty() ? rootNote : contextNotes.back().note;
        double currentBeat = contextNotes.empty() ? 0.0 : contextNotes.back().startBeat + contextNotes.back().lengthBeats;

        // Select melodic pattern based on output (if available)
        size_t patternIndex = 0;
        if (!output.empty())
        {
            patternIndex = static_cast<size_t>(juce::jlimit(0, static_cast<int>(melodicPatterns.size() - 1),
                                                           static_cast<int>(output[0] * melodicPatterns.size())));
        }

        const std::vector<int>& pattern = melodicPatterns[patternIndex];
        size_t patternStep = 0;

        for (int i = 0; i < numNotes && currentBeat < lengthBeats; ++i)
        {
            daw::project::Pattern::MIDINote note;

            // Apply pattern interval
            const int intervalOffset = pattern[patternStep % pattern.size()];

            // Find scale degree
            int scaleDegree = 0;
            for (size_t j = 0; j < scaleIntervals.size(); ++j)
            {
                if ((currentNote % 12 - rootClass + 12) % 12 == scaleIntervals[j])
                {
                    scaleDegree = static_cast<int>(j);
                    break;
                }
            }

            // Move to next scale degree with pattern offset
            scaleDegree = (scaleDegree + intervalOffset + static_cast<int>(scaleIntervals.size())) % static_cast<int>(scaleIntervals.size());
            const int targetClass = (rootClass + scaleIntervals[scaleDegree]) % 12;

            // Find nearest note in target class
            const int octave = currentNote / 12;
            int targetNote = octave * 12 + targetClass;

            // Adjust octave if too far
            if (std::abs(targetNote - currentNote) > 6)
            {
                targetNote += (targetNote < currentNote) ? 12 : -12;
            }

            note.note = static_cast<uint8_t>(juce::jlimit(0, 127, targetNote));

            // Velocity based on position in phrase
            const double phrasePos = currentBeat / lengthBeats;
            const float baseVelocity = contextNotes.empty() ? 100.0f :
                                      static_cast<float>(contextNotes.back().velocity);
            note.velocity = static_cast<uint8_t>(juce::jlimit(40, 127,
                            static_cast<int>(baseVelocity * (0.8f + 0.2f * std::sin(phrasePos * juce::MathConstants<float>::twoPi)))));

            note.startBeat = currentBeat;
            note.lengthBeats = noteDuration;
            note.channel = 0;

            result.notes.push_back(note);

            currentNote = targetNote;
            currentBeat += noteDuration;
            ++patternStep;
        }

        callback(result);
    };

    inferenceEngine->queueInference(request);
}

} // namespace daw::ai::models
