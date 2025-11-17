#pragma once

#include "Track.h"
#include "Clip.h"
#include "SelectionModel.h"
#include <vector>
#include <memory>
#include <functional>

namespace daw::project
{

/**
 * @brief Project model container
 * 
 * Manages tracks and clips in the project.
 * Provides methods for adding/removing tracks and clips.
 */
class ProjectModel
{
public:
    ProjectModel();
    ~ProjectModel() = default;

    // Non-copyable, movable
    ProjectModel(const ProjectModel&) = delete;
    ProjectModel& operator=(const ProjectModel&) = delete;
    ProjectModel(ProjectModel&&) noexcept = default;
    ProjectModel& operator=(ProjectModel&&) noexcept = default;

    // Track management
    Track* addTrack(const std::string& name, juce::Colour color);
    void removeTrack(uint32_t trackId);
    Track* getTrack(uint32_t trackId);
    [[nodiscard]] const Track* getTrack(uint32_t trackId) const;
    [[nodiscard]] std::vector<Track*> getTracks();
    [[nodiscard]] std::vector<const Track*> getTracks() const;

    // Clip management
    Clip* addClip(uint32_t trackId, double startBeats, double lengthBeats, const std::string& label);
    void removeClip(uint32_t clipId);
    Clip* getClip(uint32_t clipId);
    [[nodiscard]] const Clip* getClip(uint32_t clipId) const;
    [[nodiscard]] std::vector<Clip*> getClips();
    [[nodiscard]] std::vector<const Clip*> getClips() const;
    [[nodiscard]] std::vector<Clip*> getClipsForTrack(uint32_t trackId);

    // Selection model access
    [[nodiscard]] SelectionModel& getSelectionModel() { return selectionModel; }
    [[nodiscard]] const SelectionModel& getSelectionModel() const { return selectionModel; }

    // Listener support
    using ModelChangedCallback = std::function<void()>;
    void addModelListener(ModelChangedCallback callback);
    void removeModelListener(ModelChangedCallback callback);

private:
    std::vector<std::unique_ptr<Track>> tracks;
    std::vector<std::unique_ptr<Clip>> clips;
    SelectionModel selectionModel;
    std::vector<ModelChangedCallback> listeners;
    
    void notifyListeners();
};

} // namespace daw::project

