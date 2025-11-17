#pragma once

#include "../inference/InferenceEngine.h"
#include "../../project/Pattern.h"
#include <vector>
#include <memory>
#include <functional>

namespace daw::ai::models
{

/**
 * @brief Melody continuation and generation
 * 
 * Generates melody continuations with style matching.
 * Follows DAW_DEV_RULES: runs on background thread, non-blocking.
 */
class MelodyGenerator
{
public:
    struct MelodyResult
    {
        std::vector<daw::project::Pattern::MIDINote> notes;
        std::string style{"default"};
        bool success{false};
    };

    explicit MelodyGenerator(std::shared_ptr<daw::ai::inference::InferenceEngine> engine);
    ~MelodyGenerator() = default;

    /**
     * @brief Generate melody continuation
     * @param contextNotes Existing notes for context
     * @param style Style to match
     * @param lengthBeats Length of generated melody in beats
     * @param callback Callback with result
     */
    void generateMelody(const std::vector<daw::project::Pattern::MIDINote>& contextNotes,
                         const std::string& style, double lengthBeats,
                         std::function<void(MelodyResult)> callback);

private:
    std::shared_ptr<daw::ai::inference::InferenceEngine> inferenceEngine;
};

} // namespace daw::ai::models

