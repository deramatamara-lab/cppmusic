#pragma once
/**
 * @file AutomationVersionStore.hpp
 * @brief Version snapshots for automation undo/comparison.
 */

#include "AutomationClip.hpp"
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace cppmusic::engine::automation {

/**
 * @brief Unique identifier for automation versions.
 */
using VersionId = std::uint64_t;

/**
 * @brief Invalid version ID sentinel value.
 */
constexpr VersionId InvalidVersionId = 0;

/**
 * @brief A snapshot of automation state at a point in time.
 */
struct AutomationSnapshot {
    VersionId id = InvalidVersionId;
    std::chrono::system_clock::time_point timestamp;
    std::uint64_t contentHash = 0;
    std::vector<std::uint8_t> data;
    std::string description;
};

/**
 * @brief Represents the difference between two automation versions.
 */
struct AutomationDelta {
    VersionId fromVersion = InvalidVersionId;
    VersionId toVersion = InvalidVersionId;
    std::size_t pointsAdded = 0;
    std::size_t pointsRemoved = 0;
    std::size_t pointsModified = 0;
    bool macroChanged = false;
    bool overridesChanged = false;
};

/**
 * @brief Storage for versioned automation snapshots.
 * 
 * Maintains history of automation changes for:
 * - Undo/redo functionality
 * - Version comparison
 * - Deterministic merge operations
 */
class AutomationVersionStore {
public:
    AutomationVersionStore();
    ~AutomationVersionStore();
    
    // Non-copyable, non-movable
    AutomationVersionStore(const AutomationVersionStore&) = delete;
    AutomationVersionStore& operator=(const AutomationVersionStore&) = delete;
    AutomationVersionStore(AutomationVersionStore&&) = delete;
    AutomationVersionStore& operator=(AutomationVersionStore&&) = delete;
    
    // =========================================================================
    // Snapshot Management
    // =========================================================================
    
    /**
     * @brief Create a snapshot of the current automation state.
     * @param clip The automation clip to snapshot.
     * @param description Optional description of this version.
     * @return The version ID of the created snapshot.
     */
    VersionId createSnapshot(const AutomationClip& clip, 
                             const std::string& description = "");
    
    /**
     * @brief Restore an automation clip from a snapshot.
     * @param clip The clip to restore into.
     * @param version The version to restore.
     * @return true if the snapshot was restored.
     */
    bool restoreSnapshot(AutomationClip& clip, VersionId version);
    
    /**
     * @brief Get a snapshot by version ID.
     * @return The snapshot, or nullopt if not found.
     */
    [[nodiscard]] std::optional<AutomationSnapshot> getSnapshot(VersionId version) const;
    
    /**
     * @brief Get the latest version ID.
     */
    [[nodiscard]] VersionId getLatestVersion() const noexcept;
    
    /**
     * @brief Get all version IDs in chronological order.
     */
    [[nodiscard]] std::vector<VersionId> getAllVersions() const;
    
    // =========================================================================
    // Version Comparison
    // =========================================================================
    
    /**
     * @brief Compare two versions and get the delta.
     * @param v1 First version ID.
     * @param v2 Second version ID.
     * @return The delta between versions, or nullopt if either not found.
     */
    [[nodiscard]] std::optional<AutomationDelta> compare(VersionId v1, VersionId v2) const;
    
    /**
     * @brief Check if two versions have identical content.
     * @param v1 First version ID.
     * @param v2 Second version ID.
     * @return true if the versions have identical hashes.
     */
    [[nodiscard]] bool areVersionsIdentical(VersionId v1, VersionId v2) const;
    
    // =========================================================================
    // Storage Management
    // =========================================================================
    
    /**
     * @brief Get the number of stored snapshots.
     */
    [[nodiscard]] std::size_t getSnapshotCount() const noexcept;
    
    /**
     * @brief Prune snapshots older than the given age.
     * @param age Maximum age to keep.
     * @return Number of snapshots pruned.
     */
    std::size_t pruneOlderThan(std::chrono::seconds age);
    
    /**
     * @brief Prune all but the N most recent snapshots.
     * @param keepCount Number of snapshots to keep.
     * @return Number of snapshots pruned.
     */
    std::size_t pruneKeepRecent(std::size_t keepCount);
    
    /**
     * @brief Clear all snapshots.
     */
    void clear();
    
private:
    std::map<VersionId, AutomationSnapshot> snapshots_;
    VersionId nextVersionId_ = 1;
};

} // namespace cppmusic::engine::automation
