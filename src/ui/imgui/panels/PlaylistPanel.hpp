#pragma once

#include "../Theme.hpp"
#include <vector>
#include <string>
#include <functional>

namespace daw::ui::imgui
{

/**
 * @brief Pattern clip in the playlist
 */
struct PatternClip
{
    std::string name;
    int trackIndex{0};
    double startBeats{0.0};
    double lengthBeats{4.0};
    ImVec4 color{0.3f, 0.5f, 0.7f, 1.0f};
    bool selected{false};
    bool muted{false};
};

/**
 * @brief Playlist panel for arrangement view
 * 
 * Features:
 * - Timeline with pattern clips
 * - Track lanes
 * - Snapping and selection marquee
 * - Drag-and-drop support
 */
class PlaylistPanel
{
public:
    PlaylistPanel();
    ~PlaylistPanel() = default;

    /**
     * @brief Draw the playlist panel
     * @param open Reference to visibility flag
     * @param theme Theme for styling
     */
    void draw(bool& open, const Theme& theme);

    /**
     * @brief Get clips
     */
    [[nodiscard]] std::vector<PatternClip>& getClips() { return clips_; }

    /**
     * @brief Add a clip
     */
    void addClip(const PatternClip& clip);

    /**
     * @brief Set callback for clip selection
     */
    void setOnClipSelected(std::function<void(PatternClip*)> callback)
    {
        onClipSelected_ = std::move(callback);
    }

private:
    std::vector<PatternClip> clips_;
    std::vector<std::string> trackNames_;
    
    // View state
    double scrollX_{0.0};
    float zoomX_{1.0f};
    float pixelsPerBeat_{20.0f};
    float trackHeight_{60.0f};
    
    // Selection
    PatternClip* selectedClip_{nullptr};
    bool isDragging_{false};
    bool isSelecting_{false};
    ImVec2 selectionStart_;
    ImVec2 selectionEnd_;
    
    // Snapping
    bool snapEnabled_{true};
    int snapDivision_{4};
    
    std::function<void(PatternClip*)> onClipSelected_;

    void drawTimeline(const Theme& theme);
    void drawTracks(const Theme& theme);
    void drawClips(const Theme& theme);
    void drawSelectionMarquee(const Theme& theme);
    void drawPlayhead(const Theme& theme);
    void createDemoContent();
    
    double snapToGrid(double beats) const;
};

} // namespace daw::ui::imgui
