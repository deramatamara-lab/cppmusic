/**
 * @file ProjectController.cpp
 * @brief Implementation of ProjectController.
 */

#include "ProjectController.hpp"
#include <algorithm>
#include <unordered_set>

namespace daw::ui::core
{

ProjectController::ProjectController()
    : project_(std::make_shared<daw::project::ProjectModel>())
{
}

ProjectController::ProjectController(std::shared_ptr<daw::project::ProjectModel> project)
    : project_(std::move(project))
{
}

ProjectController::~ProjectController()
{
    listeners_.clear();
}

// =========================================================================
// Project Access
// =========================================================================

void ProjectController::setProject(std::shared_ptr<daw::project::ProjectModel> project)
{
    project_ = std::move(project);
    activePatternId_ = 0;
    notifyProjectStructureChanged();
}

void ProjectController::createNewProject()
{
    project_ = std::make_shared<daw::project::ProjectModel>();
    activePatternId_ = 0;
    notifyProjectStructureChanged();
}

// =========================================================================
// Pattern Management
// =========================================================================

std::vector<daw::project::Pattern*> ProjectController::getPatterns() const
{
    if (project_)
    {
        return project_->getPatterns();
    }
    return {};
}

std::vector<daw::project::Pattern*> ProjectController::getPatternsForTrack(uint32_t trackId) const
{
    if (!project_)
        return {};
    
    std::unordered_set<uint32_t> patternIds;
    auto clips = project_->getClipsForTrack(trackId);
    
    for (auto* clip : clips)
    {
        if (clip && clip->hasPattern())
        {
            patternIds.insert(clip->getPatternId());
        }
    }
    
    std::vector<daw::project::Pattern*> patterns;
    for (uint32_t patternId : patternIds)
    {
        if (auto* pattern = project_->getPattern(patternId))
        {
            patterns.push_back(pattern);
        }
    }
    
    return patterns;
}

daw::project::Pattern* ProjectController::createPattern(const std::string& name, int numSteps)
{
    if (project_)
    {
        auto* pattern = project_->addPattern(name, numSteps);
        notifyPatternsChanged();
        return pattern;
    }
    return nullptr;
}

void ProjectController::setActivePattern(uint32_t patternId)
{
    if (activePatternId_ != patternId)
    {
        activePatternId_ = patternId;
        notifyActivePatternChanged();
    }
}

daw::project::Pattern* ProjectController::getActivePattern() const
{
    if (project_ && activePatternId_ != 0)
    {
        return project_->getPattern(activePatternId_);
    }
    return nullptr;
}

// =========================================================================
// Track Management
// =========================================================================

std::vector<daw::project::Track*> ProjectController::getTracks() const
{
    if (project_)
    {
        return project_->getTracks();
    }
    return {};
}

daw::project::Track* ProjectController::createTrack(const std::string& name, juce::Colour color)
{
    if (project_)
    {
        auto* track = project_->addTrack(name, color);
        notifyTracksChanged();
        return track;
    }
    return nullptr;
}

// =========================================================================
// Clip Management
// =========================================================================

std::vector<daw::project::Clip*> ProjectController::getClipsForTrack(uint32_t trackId) const
{
    if (project_)
    {
        return project_->getClipsForTrack(trackId);
    }
    return {};
}

std::vector<daw::project::Clip*> ProjectController::getClipsForPattern(uint32_t patternId) const
{
    if (project_)
    {
        return project_->getClipsForPattern(patternId);
    }
    return {};
}

daw::project::Clip* ProjectController::createClip(uint32_t trackId, double startBeats, 
                                                   double lengthBeats, const std::string& label)
{
    if (project_)
    {
        auto* clip = project_->addClip(trackId, startBeats, lengthBeats, label);
        notifyClipsChanged();
        return clip;
    }
    return nullptr;
}

void ProjectController::linkClipToPattern(uint32_t clipId, uint32_t patternId)
{
    if (project_)
    {
        project_->linkClipToPattern(clipId, patternId);
        notifyClipsChanged();
    }
}

// =========================================================================
// Listener Management
// =========================================================================

void ProjectController::addListener(ProjectListener* listener)
{
    if (listener != nullptr)
    {
        auto it = std::find(listeners_.begin(), listeners_.end(), listener);
        if (it == listeners_.end())
        {
            listeners_.push_back(listener);
        }
    }
}

void ProjectController::removeListener(ProjectListener* listener)
{
    auto it = std::find(listeners_.begin(), listeners_.end(), listener);
    if (it != listeners_.end())
    {
        listeners_.erase(it);
    }
}

// =========================================================================
// Notification Helpers
// =========================================================================

void ProjectController::notifyTracksChanged()
{
    for (auto* listener : listeners_)
    {
        listener->onTracksChanged();
    }
}

void ProjectController::notifyClipsChanged()
{
    for (auto* listener : listeners_)
    {
        listener->onClipsChanged();
    }
}

void ProjectController::notifyPatternsChanged()
{
    for (auto* listener : listeners_)
    {
        listener->onPatternsChanged();
    }
}

void ProjectController::notifyActivePatternChanged()
{
    for (auto* listener : listeners_)
    {
        listener->onActivePatternChanged(activePatternId_);
    }
}

void ProjectController::notifyProjectStructureChanged()
{
    for (auto* listener : listeners_)
    {
        listener->onProjectStructureChanged();
    }
}

} // namespace daw::ui::core
