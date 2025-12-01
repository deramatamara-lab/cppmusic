#pragma once
/**
 * @file TagClassifier.hpp
 * @brief ML-based tag classification for audio samples.
 */

#include "FeatureExtractor.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace cppmusic::ai::tagging {

/**
 * @brief Tag category for classification.
 */
enum class TagCategory {
    Instrument,   ///< kick, snare, hihat, bass, synth, etc.
    Genre,        ///< electronic, rock, jazz, etc.
    Mood,         ///< dark, bright, aggressive, chill, etc.
    Texture,      ///< smooth, gritty, metallic, organic, etc.
    Technical,    ///< one-shot, loop, processed, dry, etc.
    Tempo,        ///< slow, medium, fast, variable
    Key           ///< C, C#, D, ..., B + major/minor
};

/**
 * @brief A classified tag with confidence.
 */
struct Tag {
    TagCategory category = TagCategory::Instrument;
    std::string value;
    float confidence = 0.0f;
    
    bool operator<(const Tag& other) const {
        return confidence > other.confidence;  // Sort by confidence descending
    }
};

/**
 * @brief Tag classification using ML or heuristics.
 */
class TagClassifier {
public:
    TagClassifier();
    ~TagClassifier();
    
    // =========================================================================
    // Model Loading
    // =========================================================================
    
    /**
     * @brief Load a trained classification model.
     * @param modelPath Path to model file.
     * @return true if model loaded successfully.
     */
    bool loadModel(const std::filesystem::path& modelPath);
    
    /**
     * @brief Check if a model is loaded.
     */
    [[nodiscard]] bool isModelLoaded() const noexcept { return modelLoaded_; }
    
    // =========================================================================
    // Classification
    // =========================================================================
    
    /**
     * @brief Classify features into tags.
     * @param features Audio features to classify.
     * @return Vector of tags sorted by confidence.
     */
    [[nodiscard]] std::vector<Tag> classify(const FeatureSet& features) const;
    
    /**
     * @brief Classify multiple feature sets (batch).
     */
    [[nodiscard]] std::vector<std::vector<Tag>> classifyBatch(
        const std::vector<FeatureSet>& features) const;
    
    /**
     * @brief Classify using heuristics (no model required).
     * @param features Audio features.
     * @return Vector of tags sorted by confidence.
     */
    [[nodiscard]] std::vector<Tag> classifyHeuristic(const FeatureSet& features) const;
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    /**
     * @brief Set minimum confidence threshold for returned tags.
     */
    void setConfidenceThreshold(float threshold);
    
    /**
     * @brief Get current confidence threshold.
     */
    [[nodiscard]] float getConfidenceThreshold() const noexcept { return confidenceThreshold_; }
    
    /**
     * @brief Set maximum number of tags to return.
     */
    void setMaxTags(std::size_t maxTags);
    
    /**
     * @brief Get maximum tags setting.
     */
    [[nodiscard]] std::size_t getMaxTags() const noexcept { return maxTags_; }
    
private:
    /**
     * @brief Classify instrument type from features.
     */
    [[nodiscard]] std::vector<Tag> classifyInstrument(const FeatureSet& features) const;
    
    /**
     * @brief Classify texture from features.
     */
    [[nodiscard]] std::vector<Tag> classifyTexture(const FeatureSet& features) const;
    
    /**
     * @brief Classify technical properties from features.
     */
    [[nodiscard]] std::vector<Tag> classifyTechnical(const FeatureSet& features) const;
    
    bool modelLoaded_ = false;
    float confidenceThreshold_ = 0.5f;
    std::size_t maxTags_ = 10;
};

/**
 * @brief Get string representation of tag category.
 */
[[nodiscard]] const char* toString(TagCategory category);

} // namespace cppmusic::ai::tagging
