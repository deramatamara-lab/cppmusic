#pragma once

#include <vector>
#include <cstdint>
#include <functional>

namespace daw::project
{

/**
 * @brief Selection state management
 * 
 * Manages selection of tracks and clips in the project.
 * Provides listener callbacks for selection changes.
 */
class SelectionModel
{
public:
    SelectionModel() = default;
    ~SelectionModel() = default;

    // Non-copyable, movable
    SelectionModel(const SelectionModel&) = delete;
    SelectionModel& operator=(const SelectionModel&) = delete;
    SelectionModel(SelectionModel&&) noexcept = default;
    SelectionModel& operator=(SelectionModel&&) noexcept = default;

    // Track selection
    void selectTrack(uint32_t trackId);
    void deselectTrack(uint32_t trackId);
    void clearTrackSelection();
    [[nodiscard]] bool isTrackSelected(uint32_t trackId) const;
    [[nodiscard]] std::vector<uint32_t> getSelectedTracks() const { return selectedTracks; }

    // Clip selection
    void selectClip(uint32_t clipId);
    void deselectClip(uint32_t clipId);
    void clearClipSelection();
    [[nodiscard]] bool isClipSelected(uint32_t clipId) const;
    [[nodiscard]] std::vector<uint32_t> getSelectedClips() const { return selectedClips; }

    // Clear all selections
    void clearAll();

    // Listener support
    using SelectionChangedCallback = std::function<void()>;
    void addSelectionListener(SelectionChangedCallback callback);
    void removeSelectionListener(SelectionChangedCallback callback);

private:
    std::vector<uint32_t> selectedTracks;
    std::vector<uint32_t> selectedClips;
    std::vector<SelectionChangedCallback> listeners;
    
    void notifyListeners();
};

} // namespace daw::project

