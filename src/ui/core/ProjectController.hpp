#pragma once
/**
 * @file ProjectController.hpp
 * @brief Controller managing ProjectModel and notifying UI of changes.
 *
 * The ProjectController provides a clean interface for UI components to
 * interact with the project model, managing patterns, tracks, and clips
 * while notifying listeners of changes.
 */

#include "../../project/ProjectModel.h"
#include <memory>
#include <functional>
#include <vector>

namespace daw::ui::core
{

/**
 * @brief Listener interface for project model changes.
 */
class ProjectListener
{
public:
    virtual ~ProjectListener() = default;
    
    /// Called when a track is added or removed
    virtual void onTracksChanged() {}
    
    /// Called when a clip is added, removed, or modified
    virtual void onClipsChanged() {}
    
    /// Called when a pattern is added, removed, or modified
    virtual void onPatternsChanged() {}
    
    /// Called when the active pattern for editing changes
    virtual void onActivePatternChanged(uint32_t patternId) { juce::ignoreUnused(patternId); }
    
    /// Called when project structure changes significantly
    virtual void onProjectStructureChanged() {}
};

/**
 * @brief Controller managing ProjectModel and UI notifications.
 *
 * This controller:
 * - Owns or references a ProjectModel instance
 * - Provides helper methods for common operations
 * - Notifies registered listeners when the project changes
 * - Manages the active pattern for piano roll editing
 */
class ProjectController
{
public:
    /**
     * @brief Construct a ProjectController with a new project.
     */
    ProjectController();
    
    /**
     * @brief Construct a ProjectController with an existing project.
     * @param project The project model to manage.
     */
    explicit ProjectController(std::shared_ptr<daw::project::ProjectModel> project);
    
    ~ProjectController();

    // Non-copyable, non-movable
    ProjectController(const ProjectController&) = delete;
    ProjectController& operator=(const ProjectController&) = delete;
    ProjectController(ProjectController&&) = delete;
    ProjectController& operator=(ProjectController&&) = delete;

    // =========================================================================
    // Project Access
    // =========================================================================

    /**
     * @brief Get the managed project model.
     */
    [[nodiscard]] std::shared_ptr<daw::project::ProjectModel> getProject() const 
    { 
        return project_; 
    }
    
    /**
     * @brief Set a new project model.
     * @param project The new project model.
     */
    void setProject(std::shared_ptr<daw::project::ProjectModel> project);
    
    /**
     * @brief Create a new empty project.
     */
    void createNewProject();

    // =========================================================================
    // Pattern Management
    // =========================================================================

    /**
     * @brief Get all patterns in the project.
     */
    [[nodiscard]] std::vector<daw::project::Pattern*> getPatterns() const;
    
    /**
     * @brief Get patterns for a specific channel/track.
     * @param trackId The track ID.
     * @return Vector of patterns used by clips on this track.
     */
    [[nodiscard]] std::vector<daw::project::Pattern*> getPatternsForTrack(uint32_t trackId) const;
    
    /**
     * @brief Create a new pattern.
     * @param name Pattern name.
     * @param numSteps Number of steps (default: 16).
     * @return Pointer to the created pattern.
     */
    daw::project::Pattern* createPattern(const std::string& name, int numSteps = 16);
    
    /**
     * @brief Get the active pattern for editing.
     */
    [[nodiscard]] uint32_t getActivePatternId() const { return activePatternId_; }
    
    /**
     * @brief Set the active pattern for editing.
     * @param patternId The pattern ID to set as active.
     */
    void setActivePattern(uint32_t patternId);
    
    /**
     * @brief Get the active pattern object.
     * @return Pointer to the active pattern, or nullptr if none.
     */
    [[nodiscard]] daw::project::Pattern* getActivePattern() const;

    // =========================================================================
    // Track Management
    // =========================================================================

    /**
     * @brief Get all tracks in the project.
     */
    [[nodiscard]] std::vector<daw::project::Track*> getTracks() const;
    
    /**
     * @brief Create a new track.
     * @param name Track name.
     * @param color Track color.
     * @return Pointer to the created track.
     */
    daw::project::Track* createTrack(const std::string& name, juce::Colour color);

    // =========================================================================
    // Clip Management
    // =========================================================================

    /**
     * @brief Get clips for a specific track.
     * @param trackId The track ID.
     * @return Vector of clips on this track.
     */
    [[nodiscard]] std::vector<daw::project::Clip*> getClipsForTrack(uint32_t trackId) const;
    
    /**
     * @brief Get clips that use a specific pattern.
     * @param patternId The pattern ID.
     * @return Vector of clips using this pattern.
     */
    [[nodiscard]] std::vector<daw::project::Clip*> getClipsForPattern(uint32_t patternId) const;
    
    /**
     * @brief Create a new clip on a track.
     * @param trackId Track to add clip to.
     * @param startBeats Start position in beats.
     * @param lengthBeats Clip length in beats.
     * @param label Clip label.
     * @return Pointer to the created clip.
     */
    daw::project::Clip* createClip(uint32_t trackId, double startBeats, double lengthBeats, 
                                   const std::string& label);
    
    /**
     * @brief Link a clip to a pattern.
     * @param clipId The clip ID.
     * @param patternId The pattern ID.
     */
    void linkClipToPattern(uint32_t clipId, uint32_t patternId);

    // =========================================================================
    // Listener Management
    // =========================================================================

    /**
     * @brief Add a listener for project changes.
     * @param listener The listener to add.
     */
    void addListener(ProjectListener* listener);
    
    /**
     * @brief Remove a listener.
     * @param listener The listener to remove.
     */
    void removeListener(ProjectListener* listener);

private:
    std::shared_ptr<daw::project::ProjectModel> project_;
    std::vector<ProjectListener*> listeners_;
    uint32_t activePatternId_{0};
    
    // Notification helpers
    void notifyTracksChanged();
    void notifyClipsChanged();
    void notifyPatternsChanged();
    void notifyActivePatternChanged();
    void notifyProjectStructureChanged();
};

} // namespace daw::ui::core
