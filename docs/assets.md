# Intelligent Asset Browser & Smart Tagging

This document describes the feature extraction pipeline, tag schema, database layout, and similarity search algorithm.

## Overview

The asset browser provides:
- Automatic feature extraction from audio samples
- AI-based tag classification
- Full-text and similarity search
- Hierarchical folder organization with smart collections

## Feature Extraction Pipeline

### FeatureExtractor

Extracts audio features for classification and search:

```cpp
namespace cppmusic::ai::tagging {

class FeatureExtractor {
public:
    // Extract all features from audio file
    FeatureSet extract(const std::filesystem::path& audioPath);
    
    // Individual extractors
    float computeSpectralCentroid(const float* samples, size_t numSamples);
    float computeTransientDensity(const float* samples, size_t numSamples);
    float computeRMSEnergy(const float* samples, size_t numSamples);
    std::array<float, 13> computeMFCC(const float* samples, size_t numSamples);
    float computeZeroCrossingRate(const float* samples, size_t numSamples);
};

}
```

### Feature Set

```cpp
struct FeatureSet {
    // Spectral features
    float spectralCentroid;      // Brightness (Hz)
    float spectralRolloff;       // High-frequency content
    float spectralFlux;          // Rate of change
    float spectralFlatness;      // Noise vs tonal
    
    // Temporal features
    float transientDensity;      // Attacks per second
    float rmsEnergy;             // Overall loudness
    float zeroCrossingRate;      // Noise indicator
    float duration;              // Length in seconds
    
    // Timbral features
    std::array<float, 13> mfcc;  // Mel-frequency cepstral coefficients
    
    // Rhythm features (optional)
    std::optional<float> tempo;
    std::optional<float> rhythmStrength;
};
```

### Pipeline Stages

```
┌─────────────────────────────────────────────────────────────┐
│                 Feature Extraction Pipeline                 │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐   │
│  │ Audio Load  │────►│ Resample    │────►│ Normalize   │   │
│  │ (libsndfile)│     │ (to 22050Hz)│     │ (peak = 1)  │   │
│  └─────────────┘     └─────────────┘     └─────────────┘   │
│                                                 │           │
│                                                 ▼           │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                    FFT Analysis                     │   │
│  │  - STFT with 2048 sample window, 512 hop           │   │
│  │  - Magnitude spectrum                               │   │
│  └─────────────────────────────────────────────────────┘   │
│                             │                               │
│          ┌──────────────────┼──────────────────┐           │
│          ▼                  ▼                  ▼           │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐       │
│  │ Spectral    │   │ MFCC        │   │ Temporal    │       │
│  │ Features    │   │ Extraction  │   │ Features    │       │
│  └─────────────┘   └─────────────┘   └─────────────┘       │
│          │                  │                  │           │
│          └──────────────────┼──────────────────┘           │
│                             ▼                               │
│                    ┌─────────────┐                         │
│                    │ FeatureSet  │                         │
│                    └─────────────┘                         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Tag Schema

### Hierarchical Tag Categories

```cpp
enum class TagCategory {
    Instrument,    // kick, snare, hihat, bass, synth, etc.
    Genre,         // electronic, rock, jazz, etc.
    Mood,          // dark, bright, aggressive, chill, etc.
    Texture,       // smooth, gritty, metallic, organic, etc.
    Technical,     // one-shot, loop, processed, dry, etc.
    Tempo,         // slow, medium, fast, variable
    Key            // C, C#, D, ..., B, minor, major
};

struct Tag {
    TagCategory category;
    std::string value;
    float confidence;  // 0.0 - 1.0
};
```

### Tag Vocabulary

| Category | Example Tags |
|----------|-------------|
| Instrument | kick, snare, hihat, clap, tom, percussion, bass, sub, synth-lead, synth-pad, piano, guitar, strings, brass, vocal, fx |
| Genre | electronic, house, techno, dnb, hiphop, trap, rock, jazz, ambient, cinematic |
| Mood | dark, bright, aggressive, chill, uplifting, melancholic, energetic, mysterious |
| Texture | smooth, gritty, metallic, organic, digital, warm, cold, distorted, clean |
| Technical | one-shot, loop, processed, dry, wet, layered, mono, stereo |
| Tempo | slow (<90), medium (90-130), fast (>130), variable |
| Key | C, C#, D, D#, E, F, F#, G, G#, A, A#, B + major/minor |

## TagClassifier

### ML Inference Interface

```cpp
class TagClassifier {
public:
    // Initialize with model path
    bool loadModel(const std::filesystem::path& modelPath);
    
    // Classify features into tags
    std::vector<Tag> classify(const FeatureSet& features);
    
    // Batch classification
    std::vector<std::vector<Tag>> classifyBatch(
        const std::vector<FeatureSet>& features);
    
    // Placeholder: returns heuristic-based tags until ML model available
    std::vector<Tag> classifyHeuristic(const FeatureSet& features);
};
```

### Heuristic Classification (Stub)

Until ML models are integrated:

```cpp
std::vector<Tag> TagClassifier::classifyHeuristic(const FeatureSet& features) {
    std::vector<Tag> tags;
    
    // Brightness -> instrument hints
    if (features.spectralCentroid < 500) {
        tags.push_back({TagCategory::Instrument, "bass", 0.7f});
    } else if (features.spectralCentroid > 4000) {
        tags.push_back({TagCategory::Instrument, "hihat", 0.6f});
    }
    
    // Transient density -> percussion vs sustained
    if (features.transientDensity > 5.0f) {
        tags.push_back({TagCategory::Technical, "percussive", 0.8f});
    } else if (features.transientDensity < 1.0f) {
        tags.push_back({TagCategory::Texture, "sustained", 0.7f});
    }
    
    // Duration categorization
    if (features.duration < 1.0f) {
        tags.push_back({TagCategory::Technical, "one-shot", 0.9f});
    } else {
        tags.push_back({TagCategory::Technical, "loop", 0.7f});
    }
    
    return tags;
}
```

## Database Layout

### AssetDB

SQLite-based asset database (or in-memory map for stub):

```cpp
class AssetDB {
public:
    // Initialize database
    bool initialize(const std::filesystem::path& dbPath);
    
    // Asset management
    AssetId addAsset(const AssetInfo& info);
    bool removeAsset(AssetId id);
    std::optional<AssetInfo> getAsset(AssetId id);
    
    // Search
    std::vector<AssetInfo> search(const SearchQuery& query);
    std::vector<AssetInfo> findSimilar(AssetId id, size_t limit = 10);
    
    // Tag management
    void addTag(AssetId id, const Tag& tag);
    void removeTag(AssetId id, const std::string& tagValue);
    std::vector<Tag> getTags(AssetId id);
};
```

### Schema

```sql
CREATE TABLE assets (
    id INTEGER PRIMARY KEY,
    path TEXT NOT NULL UNIQUE,
    name TEXT NOT NULL,
    duration REAL NOT NULL,
    sample_rate INTEGER NOT NULL,
    channels INTEGER NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    modified_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE features (
    asset_id INTEGER PRIMARY KEY REFERENCES assets(id),
    spectral_centroid REAL,
    spectral_rolloff REAL,
    spectral_flux REAL,
    spectral_flatness REAL,
    transient_density REAL,
    rms_energy REAL,
    zero_crossing_rate REAL,
    mfcc BLOB  -- 13 floats serialized
);

CREATE TABLE tags (
    id INTEGER PRIMARY KEY,
    asset_id INTEGER REFERENCES assets(id),
    category TEXT NOT NULL,
    value TEXT NOT NULL,
    confidence REAL NOT NULL,
    is_manual BOOLEAN DEFAULT FALSE,
    UNIQUE(asset_id, category, value)
);

CREATE INDEX idx_tags_value ON tags(value);
CREATE INDEX idx_tags_category ON tags(category);
```

## Similarity Search Algorithm

### Distance Metric

Weighted Euclidean distance on normalized features:

```cpp
float AssetDB::computeSimilarity(const FeatureSet& a, const FeatureSet& b) {
    float distance = 0.0f;
    
    // Spectral features (weight: 0.3)
    distance += 0.3f * std::pow(
        normalize(a.spectralCentroid) - normalize(b.spectralCentroid), 2);
    
    // MFCC features (weight: 0.5)
    float mfccDist = 0.0f;
    for (size_t i = 0; i < 13; ++i) {
        mfccDist += std::pow(a.mfcc[i] - b.mfcc[i], 2);
    }
    distance += 0.5f * std::sqrt(mfccDist / 13.0f);
    
    // Temporal features (weight: 0.2)
    distance += 0.2f * std::pow(
        normalize(a.transientDensity) - normalize(b.transientDensity), 2);
    
    // Convert distance to similarity (0-1 range)
    return std::exp(-distance);
}
```

### k-NN Search

```cpp
std::vector<AssetInfo> AssetDB::findSimilar(AssetId id, size_t limit) {
    auto targetFeatures = getFeatures(id);
    if (!targetFeatures) return {};
    
    // Compute similarity to all assets
    std::vector<std::pair<float, AssetId>> similarities;
    for (const auto& [assetId, features] : allFeatures_) {
        if (assetId == id) continue;
        float sim = computeSimilarity(*targetFeatures, features);
        similarities.push_back({sim, assetId});
    }
    
    // Sort by similarity (descending)
    std::partial_sort(similarities.begin(), 
                      similarities.begin() + std::min(limit, similarities.size()),
                      similarities.end(),
                      [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Return top results
    std::vector<AssetInfo> results;
    for (size_t i = 0; i < std::min(limit, similarities.size()); ++i) {
        if (auto asset = getAsset(similarities[i].second)) {
            results.push_back(*asset);
        }
    }
    return results;
}
```

## File Layout

```
src/ai/tagging/
├── FeatureExtractor.hpp/.cpp
├── TagClassifier.hpp/.cpp
└── CMakeLists.txt

src/model/assets/
├── AssetDB.hpp/.cpp
└── CMakeLists.txt

src/ui/browser/
├── AssetBrowserWidget.cpp
└── CMakeLists.txt
```

## Future Enhancements

- Approximate nearest neighbor (ANN) for large libraries
- User feedback loop to improve classification
- Audio preview in browser
- Drag-and-drop to timeline
