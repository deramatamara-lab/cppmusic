#pragma once

#include "../inference/InferenceEngine.h"
#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace daw::ai::models
{

/**
 * @brief Harmonic analysis and chord suggestions
 * 
 * Provides chord progression suggestions with voice leading.
 * Follows DAW_DEV_RULES: runs on background thread, non-blocking.
 */
class ChordAdvisor
{
public:
    struct ChordSuggestion
    {
        std::string name; // e.g., "Cmaj7"
        std::vector<int> notes; // MIDI note numbers
        float confidence{0.0f};
    };

    explicit ChordAdvisor(std::shared_ptr<daw::ai::inference::InferenceEngine> engine);
    ~ChordAdvisor() = default;

    /**
     * @brief Get chord suggestions for context
     * @param currentNotes Current notes/chords
     * @param key Key signature
     * @param callback Callback with suggestions
     */
    void getSuggestions(const std::vector<int>& currentNotes, const std::string& key,
                        std::function<void(std::vector<ChordSuggestion>)> callback);

private:
    std::shared_ptr<daw::ai::inference::InferenceEngine> inferenceEngine;
};

} // namespace daw::ai::models

