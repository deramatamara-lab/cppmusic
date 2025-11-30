#include "ProjectModel.h"
#include <algorithm>
#include <cstdint>

namespace daw::project
{

ProjectModel::ProjectModel()
{
}

Track* ProjectModel::addTrack(const std::string& name, juce::Colour color)
{
    auto track = std::make_unique<Track>(name, color);
    auto* trackPtr = track.get();
    tracks.push_back(std::move(track));
    notifyListeners();
    return trackPtr;
}

void ProjectModel::removeTrack(uint32_t trackId)
{
    const auto it = std::find_if(tracks.begin(), tracks.end(),
                                 [trackId](const auto& track) { return track->getId() == trackId; });
    if (it != tracks.end())
    {
        // Remove all clips on this track
        clips.erase(std::remove_if(clips.begin(), clips.end(),
                                   [trackId](const auto& clip) { return clip->getTrackId() == trackId; }),
                    clips.end());

        tracks.erase(it);
        selectionModel.clearTrackSelection();
        notifyListeners();
    }
}

Track* ProjectModel::getTrack(uint32_t trackId)
{
    const auto it = std::find_if(tracks.begin(), tracks.end(),
                                 [trackId](const auto& track) { return track->getId() == trackId; });
    return it != tracks.end() ? it->get() : nullptr;
}

const Track* ProjectModel::getTrack(uint32_t trackId) const
{
    const auto it = std::find_if(tracks.begin(), tracks.end(),
                                 [trackId](const auto& track) { return track->getId() == trackId; });
    return it != tracks.end() ? it->get() : nullptr;
}

Track* ProjectModel::getTrackByIndex(int index)
{
    if (index >= 0 && index < static_cast<int>(tracks.size()))
        return tracks[static_cast<size_t>(index)].get();
    return nullptr;
}

const Track* ProjectModel::getTrackByIndex(int index) const
{
    if (index >= 0 && index < static_cast<int>(tracks.size()))
        return tracks[static_cast<size_t>(index)].get();
    return nullptr;
}

std::vector<Track*> ProjectModel::getTracks()
{
    std::vector<Track*> result;
    result.reserve(tracks.size());
    for (const auto& track : tracks)
        result.push_back(track.get());
    return result;
}

std::vector<const Track*> ProjectModel::getTracks() const
{
    std::vector<const Track*> result;
    result.reserve(tracks.size());
    for (const auto& track : tracks)
        result.push_back(track.get());
    return result;
}

Clip* ProjectModel::addClip(uint32_t trackId, double startBeats, double lengthBeats, const std::string& label)
{
    auto clip = std::make_unique<Clip>(trackId, startBeats, lengthBeats, label);
    auto* clipPtr = clip.get();
    clips.push_back(std::move(clip));
    notifyListeners();
    return clipPtr;
}

void ProjectModel::removeClip(uint32_t clipId)
{
    const auto it = std::find_if(clips.begin(), clips.end(),
                                 [clipId](const auto& clip) { return clip->getId() == clipId; });
    if (it != clips.end())
    {
        clips.erase(it);
        selectionModel.deselectClip(clipId);
        notifyListeners();
    }
}

Clip* ProjectModel::getClip(uint32_t clipId)
{
    const auto it = std::find_if(clips.begin(), clips.end(),
                                 [clipId](const auto& clip) { return clip->getId() == clipId; });
    return it != clips.end() ? it->get() : nullptr;
}

const Clip* ProjectModel::getClip(uint32_t clipId) const
{
    const auto it = std::find_if(clips.begin(), clips.end(),
                                 [clipId](const auto& clip) { return clip->getId() == clipId; });
    return it != clips.end() ? it->get() : nullptr;
}

std::vector<Clip*> ProjectModel::getClips()
{
    std::vector<Clip*> result;
    result.reserve(clips.size());
    for (const auto& clip : clips)
        result.push_back(clip.get());
    return result;
}

std::vector<const Clip*> ProjectModel::getClips() const
{
    std::vector<const Clip*> result;
    result.reserve(clips.size());
    for (const auto& clip : clips)
        result.push_back(clip.get());
    return result;
}

std::vector<Clip*> ProjectModel::getClipsForTrack(uint32_t trackId)
{
    std::vector<Clip*> result;
    for (const auto& clip : clips)
    {
        if (clip->getTrackId() == trackId)
            result.push_back(clip.get());
    }
    return result;
}

void ProjectModel::addModelListener(ModelChangedCallback callback)
{
    listeners.push_back(callback);
}

void ProjectModel::removeModelListener([[maybe_unused]] ModelChangedCallback callback)
{
    // std::function doesn't support == comparison
    // For now, we just remove null callbacks
    // In a production system, consider using a listener ID system
    listeners.erase(
        std::remove_if(listeners.begin(), listeners.end(),
                     [](const auto& listener) { return !listener; }),
        listeners.end());
}

// Pattern management
Pattern* ProjectModel::addPattern(const std::string& name, int numSteps)
{
    auto pattern = std::make_unique<Pattern>(name, numSteps);
    auto* patternPtr = pattern.get();
    patterns.push_back(std::move(pattern));
    notifyListeners();
    return patternPtr;
}

void ProjectModel::removePattern(uint32_t patternId)
{
    const auto it = std::find_if(patterns.begin(), patterns.end(),
                                 [patternId](const auto& pattern) { return pattern->getId() == patternId; });
    if (it != patterns.end())
    {
        // Unlink all clips from this pattern
        for (auto& clip : clips)
        {
            if (clip->getPatternId() == patternId)
            {
                clip->setPatternId(0);
            }
        }

        patterns.erase(it);
        notifyListeners();
    }
}

Pattern* ProjectModel::getPattern(uint32_t patternId)
{
    const auto it = std::find_if(patterns.begin(), patterns.end(),
                                 [patternId](const auto& pattern) { return pattern->getId() == patternId; });
    return it != patterns.end() ? it->get() : nullptr;
}

const Pattern* ProjectModel::getPattern(uint32_t patternId) const
{
    const auto it = std::find_if(patterns.begin(), patterns.end(),
                                 [patternId](const auto& pattern) { return pattern->getId() == patternId; });
    return it != patterns.end() ? it->get() : nullptr;
}

std::vector<Pattern*> ProjectModel::getPatterns()
{
    std::vector<Pattern*> result;
    result.reserve(patterns.size());
    for (const auto& pattern : patterns)
        result.push_back(pattern.get());
    return result;
}

std::vector<const Pattern*> ProjectModel::getPatterns() const
{
    std::vector<const Pattern*> result;
    result.reserve(patterns.size());
    for (const auto& pattern : patterns)
        result.push_back(pattern.get());
    return result;
}

bool ProjectModel::setPatternNotes(uint32_t patternId, const std::vector<Pattern::MIDINote>& notes)
{
    auto* pattern = getPattern(patternId);
    if (pattern == nullptr)
        return false;

    pattern->setNotes(notes);
    notifyListeners();
    return true;
}

// Pattern-clip association
void ProjectModel::linkClipToPattern(uint32_t clipId, uint32_t patternId)
{
    auto* clip = getClip(clipId);
    if (clip != nullptr && getPattern(patternId) != nullptr)
    {
        clip->setPatternId(patternId);
        notifyListeners();
    }
}

void ProjectModel::unlinkClipFromPattern(uint32_t clipId)
{
    auto* clip = getClip(clipId);
    if (clip != nullptr)
    {
        clip->setPatternId(0);
        notifyListeners();
    }
}

std::vector<Clip*> ProjectModel::getClipsForPattern(uint32_t patternId) const
{
    std::vector<Clip*> result;
    for (const auto& clip : clips)
    {
        if (clip->getPatternId() == patternId)
        {
            result.push_back(clip.get());
        }
    }
    return result;
}

// Container management
ClipContainer* ProjectModel::addContainer(const std::string& name, juce::Colour color)
{
    auto container = std::make_unique<ClipContainer>(name, color);
    auto* containerPtr = container.get();
    containers.push_back(std::move(container));
    notifyListeners();
    return containerPtr;
}

void ProjectModel::removeContainer(uint32_t containerId)
{
    const auto it = std::find_if(containers.begin(), containers.end(),
                                 [containerId](const auto& container) { return container->getId() == containerId; });
    if (it != containers.end())
    {
        containers.erase(it);
        notifyListeners();
    }
}

ClipContainer* ProjectModel::getContainer(uint32_t containerId)
{
    const auto it = std::find_if(containers.begin(), containers.end(),
                                 [containerId](const auto& container) { return container->getId() == containerId; });
    return it != containers.end() ? it->get() : nullptr;
}

const ClipContainer* ProjectModel::getContainer(uint32_t containerId) const
{
    const auto it = std::find_if(containers.begin(), containers.end(),
                                 [containerId](const auto& container) { return container->getId() == containerId; });
    return it != containers.end() ? it->get() : nullptr;
}

std::vector<ClipContainer*> ProjectModel::getContainers()
{
    std::vector<ClipContainer*> result;
    result.reserve(containers.size());
    for (const auto& container : containers)
        result.push_back(container.get());
    return result;
}

std::vector<const ClipContainer*> ProjectModel::getContainers() const
{
    std::vector<const ClipContainer*> result;
    result.reserve(containers.size());
    for (const auto& container : containers)
        result.push_back(container.get());
    return result;
}

ClipContainer* ProjectModel::getContainerForClip(uint32_t clipId) const
{
    for (const auto& container : containers)
    {
        if (container->containsClip(clipId))
            return container.get();
    }
    return nullptr;
}

void ProjectModel::notifyListeners()
{
    for (const auto& listener : listeners)
    {
        if (listener)
            listener();
    }
}

} // namespace daw::project

