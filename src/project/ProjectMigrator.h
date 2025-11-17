#pragma once

#include <memory>
#include <string>

#include "ProjectSerializer.h"

namespace daw::project
{

class ProjectModel;

/**
 * @brief Project version migration system
 * 
 * Handles migration between project format versions.
 * Follows DAW_DEV_RULES: automatic version migration.
 */
class ProjectMigrator
{
public:
    ProjectMigrator() = default;
    ~ProjectMigrator() = default;

    // Non-copyable, movable
    ProjectMigrator(const ProjectMigrator&) = delete;
    ProjectMigrator& operator=(const ProjectMigrator&) = delete;
    ProjectMigrator(ProjectMigrator&&) noexcept = default;
    ProjectMigrator& operator=(ProjectMigrator&&) noexcept = default;

    /**
     * @brief Migrate project from old version to current version
     * @param json JSON string of old project
     * @param fromVersion Source version
     * @return Migrated JSON string, or empty if failed
     */
    [[nodiscard]] std::string migrate(const std::string& json, int fromVersion) const;

    /**
     * @brief Check if migration is needed
     * @param version Project version
     * @return true if migration needed
     */
    [[nodiscard]] static bool needsMigration(int version) noexcept
    {
        return version < ProjectSerializer::CURRENT_VERSION;
    }

private:
    std::string migrateV0ToV1(const std::string& json) const;
};

} // namespace daw::project

