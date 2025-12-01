/**
 * @file AssetDB.cpp
 * @brief Implementation of the asset database.
 */

#include "AssetDB.hpp"
#include <algorithm>
#include <cmath>

namespace cppmusic::model::assets {

AssetDB::AssetDB() = default;
AssetDB::~AssetDB() {
    close();
}

bool AssetDB::initialize(const std::filesystem::path& dbPath) {
    // Placeholder: In-memory implementation
    // Real implementation would use SQLite
    (void)dbPath;
    
    assets_.clear();
    pathIndex_.clear();
    nextId_ = 1;
    initialized_ = true;
    
    return true;
}

void AssetDB::close() {
    initialized_ = false;
}

AssetId AssetDB::addAsset(const AssetInfo& info) {
    if (!initialized_) return InvalidAssetId;
    
    // Check for duplicate path
    if (pathIndex_.find(info.path) != pathIndex_.end()) {
        return InvalidAssetId;
    }
    
    AssetId id = nextId_++;
    
    AssetInfo newInfo = info;
    newInfo.id = id;
    newInfo.createdAt = std::chrono::system_clock::now();
    newInfo.modifiedAt = newInfo.createdAt;
    
    assets_[id] = newInfo;
    pathIndex_[info.path] = id;
    
    return id;
}

bool AssetDB::removeAsset(AssetId id) {
    auto it = assets_.find(id);
    if (it == assets_.end()) {
        return false;
    }
    
    pathIndex_.erase(it->second.path);
    assets_.erase(it);
    return true;
}

std::optional<AssetInfo> AssetDB::getAsset(AssetId id) const {
    auto it = assets_.find(id);
    if (it != assets_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<AssetInfo> AssetDB::getAssetByPath(
    const std::filesystem::path& path) const {
    
    auto it = pathIndex_.find(path);
    if (it != pathIndex_.end()) {
        return getAsset(it->second);
    }
    return std::nullopt;
}

bool AssetDB::updateAsset(const AssetInfo& info) {
    auto it = assets_.find(info.id);
    if (it == assets_.end()) {
        return false;
    }
    
    // Update path index if path changed
    if (it->second.path != info.path) {
        pathIndex_.erase(it->second.path);
        pathIndex_[info.path] = info.id;
    }
    
    AssetInfo updated = info;
    updated.modifiedAt = std::chrono::system_clock::now();
    assets_[info.id] = updated;
    
    return true;
}

std::size_t AssetDB::getAssetCount() const noexcept {
    return assets_.size();
}

void AssetDB::addTag(AssetId id, const ai::tagging::Tag& tag, bool /*isManual*/) {
    auto it = assets_.find(id);
    if (it == assets_.end()) {
        return;
    }
    
    // Check if tag already exists
    auto& tags = it->second.tags;
    auto tagIt = std::find_if(tags.begin(), tags.end(),
        [&tag](const ai::tagging::Tag& t) {
            return t.value == tag.value && t.category == tag.category;
        });
    
    if (tagIt == tags.end()) {
        tags.push_back(tag);
    } else {
        // Update confidence if new confidence is higher
        if (tag.confidence > tagIt->confidence) {
            tagIt->confidence = tag.confidence;
        }
    }
}

bool AssetDB::removeTag(AssetId id, const std::string& tagValue) {
    auto it = assets_.find(id);
    if (it == assets_.end()) {
        return false;
    }
    
    auto& tags = it->second.tags;
    auto origSize = tags.size();
    tags.erase(
        std::remove_if(tags.begin(), tags.end(),
            [&tagValue](const ai::tagging::Tag& t) { return t.value == tagValue; }),
        tags.end());
    
    return tags.size() < origSize;
}

std::vector<ai::tagging::Tag> AssetDB::getTags(AssetId id) const {
    auto it = assets_.find(id);
    if (it != assets_.end()) {
        return it->second.tags;
    }
    return {};
}

std::vector<std::string> AssetDB::getAllUniqueTags() const {
    std::vector<std::string> uniqueTags;
    
    for (const auto& [id, asset] : assets_) {
        for (const auto& tag : asset.tags) {
            if (std::find(uniqueTags.begin(), uniqueTags.end(), tag.value) == uniqueTags.end()) {
                uniqueTags.push_back(tag.value);
            }
        }
    }
    
    std::sort(uniqueTags.begin(), uniqueTags.end());
    return uniqueTags;
}

std::vector<AssetInfo> AssetDB::search(const SearchQuery& query) const {
    std::vector<AssetInfo> results;
    
    for (const auto& [id, asset] : assets_) {
        if (matchesQuery(asset, query)) {
            results.push_back(asset);
            
            if (results.size() >= query.maxResults) {
                break;
            }
        }
    }
    
    return results;
}

std::vector<AssetInfo> AssetDB::findSimilar(AssetId id, std::size_t limit) const {
    auto it = assets_.find(id);
    if (it == assets_.end()) {
        return {};
    }
    
    return findSimilarByFeatures(it->second.features, limit);
}

std::vector<AssetInfo> AssetDB::findSimilarByFeatures(
    const ai::tagging::FeatureSet& features,
    std::size_t limit) const {
    
    std::vector<std::pair<float, AssetId>> similarities;
    
    for (const auto& [assetId, asset] : assets_) {
        float sim = computeSimilarity(features, asset.features);
        similarities.push_back({sim, assetId});
    }
    
    // Sort by similarity (descending)
    std::partial_sort(similarities.begin(),
                      similarities.begin() + std::min(limit, similarities.size()),
                      similarities.end(),
                      [](const auto& a, const auto& b) { return a.first > b.first; });
    
    std::vector<AssetInfo> results;
    for (std::size_t i = 0; i < std::min(limit, similarities.size()); ++i) {
        if (auto asset = getAsset(similarities[i].second)) {
            results.push_back(*asset);
        }
    }
    
    return results;
}

std::size_t AssetDB::scanDirectory(const std::filesystem::path& directory,
                                    bool recursive) {
    std::size_t added = 0;
    
    if (!std::filesystem::exists(directory)) {
        return 0;
    }
    
    auto scanFile = [this, &added](const std::filesystem::path& path) {
        // Check if it's an audio file (by extension)
        auto ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == ".wav" || ext == ".mp3" || ext == ".flac" || 
            ext == ".aiff" || ext == ".ogg") {
            
            // Skip if already in database
            if (pathIndex_.find(path) != pathIndex_.end()) {
                return;
            }
            
            AssetInfo info;
            info.path = path;
            info.name = path.stem().string();
            info.duration = 0.0f;  // Would be extracted from file
            
            // Extract features (placeholder)
            // info.features = featureExtractor_.extractFromFile(path);
            
            // Auto-tag (placeholder)
            // info.tags = tagClassifier_.classify(info.features);
            
            if (addAsset(info) != InvalidAssetId) {
                ++added;
            }
        }
    };
    
    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                scanFile(entry.path());
            }
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                scanFile(entry.path());
            }
        }
    }
    
    return added;
}

void AssetDB::regenerateAllTags() {
    for (auto& [id, asset] : assets_) {
        // Clear auto-generated tags
        asset.tags.erase(
            std::remove_if(asset.tags.begin(), asset.tags.end(),
                [](const ai::tagging::Tag& t) { return t.confidence < 1.0f; }),
            asset.tags.end());
        
        // Regenerate from features
        auto newTags = tagClassifier_.classify(asset.features);
        for (const auto& tag : newTags) {
            addTag(id, tag, false);
        }
    }
}

float AssetDB::computeSimilarity(const ai::tagging::FeatureSet& a,
                                  const ai::tagging::FeatureSet& b) const {
    float distance = 0.0f;
    
    // Spectral centroid (normalized)
    float maxCentroid = 10000.0f;
    distance += std::pow((a.spectralCentroid - b.spectralCentroid) / maxCentroid, 2) * 0.2f;
    
    // RMS energy
    distance += std::pow(a.rmsEnergy - b.rmsEnergy, 2) * 0.1f;
    
    // Transient density (normalized)
    float maxTransient = 20.0f;
    distance += std::pow((a.transientDensity - b.transientDensity) / maxTransient, 2) * 0.2f;
    
    // MFCC similarity (simplified)
    float mfccDist = 0.0f;
    for (std::size_t i = 0; i < 13; ++i) {
        mfccDist += std::pow(a.mfcc[i] - b.mfcc[i], 2);
    }
    distance += std::sqrt(mfccDist / 13.0f) * 0.5f;
    
    // Convert distance to similarity (0-1)
    return std::exp(-distance);
}

bool AssetDB::matchesQuery(const AssetInfo& asset, const SearchQuery& query) const {
    // Text search in name
    if (!query.textQuery.empty()) {
        std::string lowerName = asset.name;
        std::string lowerQuery = query.textQuery;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
        
        if (lowerName.find(lowerQuery) == std::string::npos) {
            // Also search in tags
            bool foundInTags = false;
            for (const auto& tag : asset.tags) {
                std::string lowerTag = tag.value;
                std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
                if (lowerTag.find(lowerQuery) != std::string::npos) {
                    foundInTags = true;
                    break;
                }
            }
            if (!foundInTags) return false;
        }
    }
    
    // Required tags
    for (const auto& required : query.requiredTags) {
        bool found = false;
        for (const auto& tag : asset.tags) {
            if (tag.value == required) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    
    // Excluded tags
    for (const auto& excluded : query.excludedTags) {
        for (const auto& tag : asset.tags) {
            if (tag.value == excluded) {
                return false;
            }
        }
    }
    
    // Duration filters
    if (query.minDuration && asset.duration < *query.minDuration) {
        return false;
    }
    if (query.maxDuration && asset.duration > *query.maxDuration) {
        return false;
    }
    
    // Category filter
    if (query.category) {
        bool hasCategory = false;
        for (const auto& tag : asset.tags) {
            if (tag.category == *query.category) {
                hasCategory = true;
                break;
            }
        }
        if (!hasCategory) return false;
    }
    
    return true;
}

} // namespace cppmusic::model::assets
