#include "SelectionModel.h"
#include <algorithm>

namespace daw::project
{

void SelectionModel::selectTrack(uint32_t trackId)
{
    if (std::find(selectedTracks.begin(), selectedTracks.end(), trackId) == selectedTracks.end())
    {
        selectedTracks.push_back(trackId);
        notifyListeners();
    }
}

void SelectionModel::deselectTrack(uint32_t trackId)
{
    const auto it = std::find(selectedTracks.begin(), selectedTracks.end(), trackId);
    if (it != selectedTracks.end())
    {
        selectedTracks.erase(it);
        notifyListeners();
    }
}

void SelectionModel::clearTrackSelection()
{
    if (!selectedTracks.empty())
    {
        selectedTracks.clear();
        notifyListeners();
    }
}

bool SelectionModel::isTrackSelected(uint32_t trackId) const
{
    return std::find(selectedTracks.begin(), selectedTracks.end(), trackId) != selectedTracks.end();
}

void SelectionModel::selectClip(uint32_t clipId)
{
    if (std::find(selectedClips.begin(), selectedClips.end(), clipId) == selectedClips.end())
    {
        selectedClips.push_back(clipId);
        notifyListeners();
    }
}

void SelectionModel::deselectClip(uint32_t clipId)
{
    const auto it = std::find(selectedClips.begin(), selectedClips.end(), clipId);
    if (it != selectedClips.end())
    {
        selectedClips.erase(it);
        notifyListeners();
    }
}

void SelectionModel::clearClipSelection()
{
    if (!selectedClips.empty())
    {
        selectedClips.clear();
        notifyListeners();
    }
}

bool SelectionModel::isClipSelected(uint32_t clipId) const
{
    return std::find(selectedClips.begin(), selectedClips.end(), clipId) != selectedClips.end();
}

void SelectionModel::clearAll()
{
    if (!selectedTracks.empty() || !selectedClips.empty())
    {
        selectedTracks.clear();
        selectedClips.clear();
        notifyListeners();
    }
}

void SelectionModel::addSelectionListener(SelectionChangedCallback callback)
{
    listeners.push_back(callback);
}

void SelectionModel::removeSelectionListener([[maybe_unused]] SelectionChangedCallback callback)
{
    // std::function doesn't support == comparison
    // For now, we just remove null callbacks
    // In a production system, consider using a listener ID system
    listeners.erase(
        std::remove_if(listeners.begin(), listeners.end(),
                     [](const auto& listener) { return !listener; }),
        listeners.end());
}

void SelectionModel::notifyListeners()
{
    for (const auto& listener : listeners)
    {
        if (listener)
            listener();
    }
}

} // namespace daw::project

