#pragma once

#include "ProjectModel.h"
#include <string>
#include <memory>
#include <vector>

namespace daw::project
{

/**
 * @brief Project serialization
 * 
 * Serializes/deserializes project to/from JSON format with versioning.
 * Follows DAW_DEV_RULES: versioned format, migration support.
 */
class ProjectSerializer
{
public:
    static constexpr int CURRENT_VERSION = 1;

    ProjectSerializer() = default;
    ~ProjectSerializer() = default;

    // Non-copyable, movable
    ProjectSerializer(const ProjectSerializer&) = delete;
    ProjectSerializer& operator=(const ProjectSerializer&) = delete;
    ProjectSerializer(ProjectSerializer&&) noexcept = default;
    ProjectSerializer& operator=(ProjectSerializer&&) noexcept = default;

    /**
     * @brief Serialize project to JSON string
     * @param model Project model to serialize
     * @return JSON string
     */
    [[nodiscard]] std::string serialize(const ProjectModel& model) const;

    /**
     * @brief Deserialize project from JSON string
     * @param json JSON string
     * @return Project model, or nullptr if failed
     */
    [[nodiscard]] std::unique_ptr<ProjectModel> deserialize(const std::string& json) const;

    /**
     * @brief Save project to file
     * @param model Project model
     * @param filePath File path
     * @return true if successful
     */
    bool saveToFile(const ProjectModel& model, const std::string& filePath) const;

    /**
     * @brief Load project from file
     * @param filePath File path
     * @return Project model, or nullptr if failed
     */
    [[nodiscard]] std::unique_ptr<ProjectModel> loadFromFile(const std::string& filePath) const;

    /**
     * @brief Get project version from file
     * @param filePath File path
     * @return Version number, or -1 if failed
     */
    [[nodiscard]] int getVersionFromFile(const std::string& filePath) const;

private:
    // Helper methods for JSON serialization
    std::string serializeTrack(const Track& track) const;
    std::string serializeClip(const Clip& clip) const;
    
    Track* deserializeTrack(const std::string& json) const;
    Clip* deserializeClip(const std::string& json) const;
};

} // namespace daw::project

