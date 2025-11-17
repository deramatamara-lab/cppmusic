#include "ProjectModel.h"
#include <algorithm>

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

void ProjectModel::notifyListeners()
{
    for (const auto& listener : listeners)
    {
        if (listener)
            listener();
    }
}

} // namespace daw::project

