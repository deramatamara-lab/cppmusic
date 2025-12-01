/**
 * @file TagClassifier.cpp
 * @brief Implementation of tag classification.
 */

#include "TagClassifier.hpp"
#include <algorithm>

namespace cppmusic::ai::tagging {

const char* toString(TagCategory category) {
    switch (category) {
        case TagCategory::Instrument: return "Instrument";
        case TagCategory::Genre: return "Genre";
        case TagCategory::Mood: return "Mood";
        case TagCategory::Texture: return "Texture";
        case TagCategory::Technical: return "Technical";
        case TagCategory::Tempo: return "Tempo";
        case TagCategory::Key: return "Key";
    }
    return "Unknown";
}

TagClassifier::TagClassifier() = default;
TagClassifier::~TagClassifier() = default;

bool TagClassifier::loadModel(const std::filesystem::path& /*modelPath*/) {
    // Placeholder: Would load ONNX/TensorFlow/custom model
    // For now, just mark as not loaded to use heuristics
    modelLoaded_ = false;
    return false;
}

std::vector<Tag> TagClassifier::classify(const FeatureSet& features) const {
    if (modelLoaded_) {
        // Use ML model for classification
        // Placeholder: Fall through to heuristics
    }
    
    return classifyHeuristic(features);
}

std::vector<std::vector<Tag>> TagClassifier::classifyBatch(
    const std::vector<FeatureSet>& features) const {
    
    std::vector<std::vector<Tag>> results;
    results.reserve(features.size());
    
    for (const auto& feat : features) {
        results.push_back(classify(feat));
    }
    
    return results;
}

std::vector<Tag> TagClassifier::classifyHeuristic(const FeatureSet& features) const {
    std::vector<Tag> tags;
    
    if (!features.isValid()) {
        return tags;
    }
    
    // Collect tags from different classifiers
    auto instrumentTags = classifyInstrument(features);
    auto textureTags = classifyTexture(features);
    auto technicalTags = classifyTechnical(features);
    
    tags.insert(tags.end(), instrumentTags.begin(), instrumentTags.end());
    tags.insert(tags.end(), textureTags.begin(), textureTags.end());
    tags.insert(tags.end(), technicalTags.begin(), technicalTags.end());
    
    // Filter by confidence threshold
    tags.erase(std::remove_if(tags.begin(), tags.end(),
        [this](const Tag& t) { return t.confidence < confidenceThreshold_; }),
        tags.end());
    
    // Sort by confidence
    std::sort(tags.begin(), tags.end());
    
    // Limit number of tags
    if (tags.size() > maxTags_) {
        tags.resize(maxTags_);
    }
    
    return tags;
}

void TagClassifier::setConfidenceThreshold(float threshold) {
    confidenceThreshold_ = std::clamp(threshold, 0.0f, 1.0f);
}

void TagClassifier::setMaxTags(std::size_t maxTags) {
    maxTags_ = maxTags;
}

std::vector<Tag> TagClassifier::classifyInstrument(const FeatureSet& features) const {
    std::vector<Tag> tags;
    
    // Low spectral centroid -> bass content
    if (features.spectralCentroid < 500.0f) {
        tags.push_back({TagCategory::Instrument, "bass", 0.7f});
        
        if (features.transientDensity > 2.0f) {
            tags.push_back({TagCategory::Instrument, "kick", 0.6f});
        } else {
            tags.push_back({TagCategory::Instrument, "sub", 0.5f});
        }
    }
    
    // High spectral centroid -> bright content
    if (features.spectralCentroid > 4000.0f) {
        tags.push_back({TagCategory::Instrument, "hihat", 0.6f});
        
        if (features.spectralFlatness > 0.5f) {
            tags.push_back({TagCategory::Instrument, "crash", 0.5f});
        }
    }
    
    // Mid-range with transients -> snare
    if (features.spectralCentroid > 1000.0f && features.spectralCentroid < 4000.0f) {
        if (features.transientDensity > 1.0f && features.transientDensity < 10.0f) {
            tags.push_back({TagCategory::Instrument, "snare", 0.5f});
        }
    }
    
    // High zero crossing rate -> noise/texture
    if (features.zeroCrossingRate > 3000.0f) {
        tags.push_back({TagCategory::Instrument, "noise", 0.6f});
    }
    
    // Low zero crossing, low transient -> pad/sustained
    if (features.zeroCrossingRate < 500.0f && features.transientDensity < 1.0f) {
        tags.push_back({TagCategory::Instrument, "pad", 0.5f});
        tags.push_back({TagCategory::Instrument, "synth", 0.4f});
    }
    
    return tags;
}

std::vector<Tag> TagClassifier::classifyTexture(const FeatureSet& features) const {
    std::vector<Tag> tags;
    
    // High spectral flatness -> noisy/gritty
    if (features.spectralFlatness > 0.6f) {
        tags.push_back({TagCategory::Texture, "gritty", 0.7f});
        tags.push_back({TagCategory::Texture, "noisy", 0.6f});
    } else if (features.spectralFlatness < 0.2f) {
        tags.push_back({TagCategory::Texture, "tonal", 0.7f});
        tags.push_back({TagCategory::Texture, "clean", 0.6f});
    }
    
    // Smooth = low transient density + low zero crossing
    if (features.transientDensity < 2.0f && features.zeroCrossingRate < 1000.0f) {
        tags.push_back({TagCategory::Texture, "smooth", 0.6f});
    }
    
    // Aggressive = high transient + high energy
    if (features.transientDensity > 5.0f && features.rmsEnergy > 0.3f) {
        tags.push_back({TagCategory::Texture, "aggressive", 0.6f});
    }
    
    // Warm vs cold based on spectral centroid and flatness
    if (features.spectralCentroid < 2000.0f && features.spectralFlatness < 0.3f) {
        tags.push_back({TagCategory::Texture, "warm", 0.5f});
    } else if (features.spectralCentroid > 3000.0f) {
        tags.push_back({TagCategory::Texture, "bright", 0.5f});
    }
    
    return tags;
}

std::vector<Tag> TagClassifier::classifyTechnical(const FeatureSet& features) const {
    std::vector<Tag> tags;
    
    // Duration-based classification
    if (features.duration < 1.0f) {
        tags.push_back({TagCategory::Technical, "one-shot", 0.9f});
    } else if (features.duration >= 1.0f && features.duration < 8.0f) {
        tags.push_back({TagCategory::Technical, "short-loop", 0.7f});
    } else {
        tags.push_back({TagCategory::Technical, "loop", 0.7f});
    }
    
    // Percussive vs sustained
    if (features.transientDensity > 5.0f) {
        tags.push_back({TagCategory::Technical, "percussive", 0.8f});
    } else if (features.transientDensity < 1.0f) {
        tags.push_back({TagCategory::Technical, "sustained", 0.7f});
    }
    
    // Processed indicator (high variation in features)
    float mfccVariance = 0.0f;
    for (const auto& m : features.mfcc) {
        mfccVariance += m * m;
    }
    
    if (mfccVariance > 1.0f) {
        tags.push_back({TagCategory::Technical, "processed", 0.5f});
    } else {
        tags.push_back({TagCategory::Technical, "dry", 0.5f});
    }
    
    return tags;
}

} // namespace cppmusic::ai::tagging
