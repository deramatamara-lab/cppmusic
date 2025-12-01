/**
 * @file AutomationVersionStore.cpp
 * @brief Implementation of the automation version store.
 */

#include "AutomationVersionStore.hpp"

namespace cppmusic::engine::automation {

AutomationVersionStore::AutomationVersionStore() = default;
AutomationVersionStore::~AutomationVersionStore() = default;

VersionId AutomationVersionStore::createSnapshot(const AutomationClip& clip,
                                                  const std::string& description) {
    VersionId id = nextVersionId_++;
    
    AutomationSnapshot snapshot;
    snapshot.id = id;
    snapshot.timestamp = std::chrono::system_clock::now();
    snapshot.contentHash = clip.computeHash();
    snapshot.data = clip.serialize();
    snapshot.description = description;
    
    snapshots_[id] = std::move(snapshot);
    
    return id;
}

bool AutomationVersionStore::restoreSnapshot(AutomationClip& clip, VersionId version) {
    auto it = snapshots_.find(version);
    if (it == snapshots_.end()) {
        return false;
    }
    
    clip = AutomationClip::deserialize(it->second.data);
    return true;
}

std::optional<AutomationSnapshot> AutomationVersionStore::getSnapshot(VersionId version) const {
    auto it = snapshots_.find(version);
    if (it != snapshots_.end()) {
        return it->second;
    }
    return std::nullopt;
}

VersionId AutomationVersionStore::getLatestVersion() const noexcept {
    if (snapshots_.empty()) {
        return InvalidVersionId;
    }
    return snapshots_.rbegin()->first;
}

std::vector<VersionId> AutomationVersionStore::getAllVersions() const {
    std::vector<VersionId> versions;
    versions.reserve(snapshots_.size());
    for (const auto& [id, snapshot] : snapshots_) {
        versions.push_back(id);
    }
    return versions;
}

std::optional<AutomationDelta> AutomationVersionStore::compare(VersionId v1, VersionId v2) const {
    auto it1 = snapshots_.find(v1);
    auto it2 = snapshots_.find(v2);
    
    if (it1 == snapshots_.end() || it2 == snapshots_.end()) {
        return std::nullopt;
    }
    
    AutomationDelta delta;
    delta.fromVersion = v1;
    delta.toVersion = v2;
    
    // Deserialize both clips for comparison
    AutomationClip clip1 = AutomationClip::deserialize(it1->second.data);
    AutomationClip clip2 = AutomationClip::deserialize(it2->second.data);
    
    // Simple comparison: count point differences
    std::size_t count1 = clip1.getPointCount();
    std::size_t count2 = clip2.getPointCount();
    
    if (count2 > count1) {
        delta.pointsAdded = count2 - count1;
    } else if (count1 > count2) {
        delta.pointsRemoved = count1 - count2;
    }
    
    // Check if any points are modified (simplified check)
    std::size_t minCount = std::min(count1, count2);
    const auto& points1 = clip1.getPoints();
    const auto& points2 = clip2.getPoints();
    
    for (std::size_t i = 0; i < minCount; ++i) {
        if (points1[i].beat != points2[i].beat ||
            points1[i].value != points2[i].value) {
            delta.pointsModified++;
        }
    }
    
    // Check macro changes
    delta.macroChanged = (clip1.isMacroEnabled() != clip2.isMacroEnabled());
    
    // Check override changes
    delta.overridesChanged = (clip1.getOverrides().size() != clip2.getOverrides().size());
    
    return delta;
}

bool AutomationVersionStore::areVersionsIdentical(VersionId v1, VersionId v2) const {
    auto it1 = snapshots_.find(v1);
    auto it2 = snapshots_.find(v2);
    
    if (it1 == snapshots_.end() || it2 == snapshots_.end()) {
        return false;
    }
    
    return it1->second.contentHash == it2->second.contentHash;
}

std::size_t AutomationVersionStore::getSnapshotCount() const noexcept {
    return snapshots_.size();
}

std::size_t AutomationVersionStore::pruneOlderThan(std::chrono::seconds age) {
    auto cutoff = std::chrono::system_clock::now() - age;
    std::size_t pruned = 0;
    
    for (auto it = snapshots_.begin(); it != snapshots_.end(); ) {
        if (it->second.timestamp < cutoff) {
            it = snapshots_.erase(it);
            ++pruned;
        } else {
            ++it;
        }
    }
    
    return pruned;
}

std::size_t AutomationVersionStore::pruneKeepRecent(std::size_t keepCount) {
    if (snapshots_.size() <= keepCount) {
        return 0;
    }
    
    std::size_t toRemove = snapshots_.size() - keepCount;
    std::size_t pruned = 0;
    
    for (auto it = snapshots_.begin(); it != snapshots_.end() && pruned < toRemove; ) {
        it = snapshots_.erase(it);
        ++pruned;
    }
    
    return pruned;
}

void AutomationVersionStore::clear() {
    snapshots_.clear();
}

} // namespace cppmusic::engine::automation
