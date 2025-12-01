#pragma once
/**
 * @file AssetDB.hpp
 * @brief Database for audio asset management and search.
 */

#include "ai/tagging/FeatureExtractor.hpp"
#include "ai/tagging/TagClassifier.hpp"
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace cppmusic::model::assets {

/**
 * @brief Unique identifier for an asset.
 */
using AssetId = std::uint64_t;

/**
 * @brief Invalid asset ID sentinel value.
 */
constexpr AssetId InvalidAssetId = 0;

/**
 * @brief Information about an audio asset.
 */
struct AssetInfo {
    AssetId id = InvalidAssetId;
    std::filesystem::path path;
    std::string name;
    float duration = 0.0f;           ///< Duration in seconds
    std::uint32_t sampleRate = 44100;
    std::uint32_t channels = 2;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point modifiedAt;
    
    // Extracted features
    ai::tagging::FeatureSet features;
    
    // Tags
    std::vector<ai::tagging::Tag> tags;
};

/**
 * @brief Search query for assets.
 */
struct SearchQuery {
    std::string textQuery;                        ///< Free-text search
    std::vector<std::string> requiredTags;        ///< Must have these tags
    std::vector<std::string> excludedTags;        ///< Must not have these tags
    std::optional<float> minDuration;
    std::optional<float> maxDuration;
    std::optional<ai::tagging::TagCategory> category;
    std::size_t maxResults = 100;
};

/**
 * @brief Database for audio assets with search and tagging.
 * 
 * Provides:
 * - Asset registration and metadata storage
 * - Tag management (auto and manual)
 * - Text and similarity search
 * - Feature-based similarity matching
 */
class AssetDB {
public:
    AssetDB();
    ~AssetDB();
    
    // Non-copyable, non-movable
    AssetDB(const AssetDB&) = delete;
    AssetDB& operator=(const AssetDB&) = delete;
    AssetDB(AssetDB&&) = delete;
    AssetDB& operator=(AssetDB&&) = delete;
    
    // =========================================================================
    // Database Management
    // =========================================================================
    
    /**
     * @brief Initialize database (SQLite or in-memory).
     * @param dbPath Path to database file, or empty for in-memory.
     */
    bool initialize(const std::filesystem::path& dbPath = "");
    
    /**
     * @brief Close database connection.
     */
    void close();
    
    /**
     * @brief Check if database is initialized.
     */
    [[nodiscard]] bool isInitialized() const noexcept { return initialized_; }
    
    // =========================================================================
    // Asset Management
    // =========================================================================
    
    /**
     * @brief Add an asset to the database.
     * @param info Asset information.
     * @return The assigned asset ID, or InvalidAssetId on failure.
     */
    AssetId addAsset(const AssetInfo& info);
    
    /**
     * @brief Remove an asset from the database.
     */
    bool removeAsset(AssetId id);
    
    /**
     * @brief Get asset by ID.
     */
    [[nodiscard]] std::optional<AssetInfo> getAsset(AssetId id) const;
    
    /**
     * @brief Get asset by path.
     */
    [[nodiscard]] std::optional<AssetInfo> getAssetByPath(
        const std::filesystem::path& path) const;
    
    /**
     * @brief Update asset information.
     */
    bool updateAsset(const AssetInfo& info);
    
    /**
     * @brief Get total number of assets.
     */
    [[nodiscard]] std::size_t getAssetCount() const noexcept;
    
    // =========================================================================
    // Tag Management
    // =========================================================================
    
    /**
     * @brief Add a tag to an asset.
     * @param isManual true for user-assigned tags, false for auto-generated.
     */
    void addTag(AssetId id, const ai::tagging::Tag& tag, bool isManual = false);
    
    /**
     * @brief Remove a tag from an asset.
     */
    bool removeTag(AssetId id, const std::string& tagValue);
    
    /**
     * @brief Get all tags for an asset.
     */
    [[nodiscard]] std::vector<ai::tagging::Tag> getTags(AssetId id) const;
    
    /**
     * @brief Get all unique tags in the database.
     */
    [[nodiscard]] std::vector<std::string> getAllUniqueTags() const;
    
    // =========================================================================
    // Search
    // =========================================================================
    
    /**
     * @brief Search for assets matching query.
     */
    [[nodiscard]] std::vector<AssetInfo> search(const SearchQuery& query) const;
    
    /**
     * @brief Find assets similar to a reference asset.
     * @param id Reference asset ID.
     * @param limit Maximum results.
     */
    [[nodiscard]] std::vector<AssetInfo> findSimilar(AssetId id, 
                                                      std::size_t limit = 10) const;
    
    /**
     * @brief Find assets similar to given features.
     */
    [[nodiscard]] std::vector<AssetInfo> findSimilarByFeatures(
        const ai::tagging::FeatureSet& features,
        std::size_t limit = 10) const;
    
    // =========================================================================
    // Batch Operations
    // =========================================================================
    
    /**
     * @brief Scan a directory and add all audio files.
     * @param directory Directory to scan.
     * @param recursive Include subdirectories.
     * @return Number of assets added.
     */
    std::size_t scanDirectory(const std::filesystem::path& directory,
                              bool recursive = true);
    
    /**
     * @brief Regenerate tags for all assets.
     */
    void regenerateAllTags();
    
private:
    /**
     * @brief Compute similarity between two feature sets.
     */
    [[nodiscard]] float computeSimilarity(const ai::tagging::FeatureSet& a,
                                          const ai::tagging::FeatureSet& b) const;
    
    /**
     * @brief Check if asset matches search query.
     */
    [[nodiscard]] bool matchesQuery(const AssetInfo& asset, 
                                    const SearchQuery& query) const;
    
    bool initialized_ = false;
    std::map<AssetId, AssetInfo> assets_;
    std::map<std::filesystem::path, AssetId> pathIndex_;
    AssetId nextId_ = 1;
    
    ai::tagging::FeatureExtractor featureExtractor_;
    ai::tagging::TagClassifier tagClassifier_;
};

} // namespace cppmusic::model::assets
