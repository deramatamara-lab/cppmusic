#pragma once

#include "../Theme.hpp"
#include <vector>
#include <string>
#include <functional>

namespace daw::ui::imgui
{

/**
 * @brief Warp marker for clip time transformation
 */
struct ClipWarpMarker
{
    double sourceBeat{0.0};   ///< Position in original clip
    double targetBeat{0.0};   ///< Position in warped clip
    bool selected{false};
};

/**
 * @brief Pattern clip in the playlist with deep-edit capabilities
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
    
    // Deep-edit properties
    bool ghosted{false};             ///< Ghost state (visible but not playing)
    int colorGroup{0};               ///< Color group for organization
    
    // Per-instance modifiers
    int transpose{0};                ///< Per-clip transpose (-24 to +24)
    float gain{1.0f};                ///< Per-clip gain (0 to 2)
    
    // Warp/stretch
    bool stretchEnabled{false};      ///< Time-stretch enabled
    double stretchRatio{1.0};        ///< Stretch ratio (0.5 = half speed, 2 = double)
    std::vector<ClipWarpMarker> warpMarkers;  ///< Warp markers for non-linear stretch
    
    // Slip editing
    double slipOffset{0.0};          ///< Content offset within clip (for slip editing)
};

/**
 * @brief Playlist tool modes
 */
enum class PlaylistTool {
    Select,     ///< Select and move clips
    Slice,      ///< Slice clips
    Slip,       ///< Slip edit (adjust content offset)
    Stretch,    ///< Time-stretch clips
    Draw        ///< Draw new clips
};

/**
 * @brief Playlist panel for arrangement view
 * 
 * Features:
 * - Timeline with pattern clips
 * - Track lanes with headers
 * - Snapping and selection marquee
 * - Clip warp/stretch with RubberBand-style interface
 * - Slip editing
 * - Per-instance transpose/gain
 * - Mute and ghost states
 * - Loop region and smooth playhead follow
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

    /**
     * @brief Set the current tool
     */
    void setCurrentTool(PlaylistTool tool) { currentTool_ = tool; }

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
    bool isSlipEditing_{false};
    bool isStretching_{false};
    ImVec2 selectionStart_;
    ImVec2 selectionEnd_;
    
    // Tool state
    PlaylistTool currentTool_{PlaylistTool::Select};
    
    // Snapping
    bool snapEnabled_{true};
    int snapDivision_{4};
    
    // Loop region
    bool loopEnabled_{false};
    double loopStart_{0.0};
    double loopEnd_{16.0};
    
    // Playhead
    double playheadPosition_{0.0};
    bool playheadFollowing_{true};
    
    std::function<void(PatternClip*)> onClipSelected_;

    void drawToolbar(const Theme& theme);
    void drawTimeline(const Theme& theme);
    void drawTracks(const Theme& theme);
    void drawClips(const Theme& theme);
    void drawClipWarpMarkers(const PatternClip& clip, const Theme& theme, ImVec2 clipPos, ImVec2 clipSize);
    void drawSelectionMarquee(const Theme& theme);
    void drawPlayhead(const Theme& theme);
    void drawLoopRegion(const Theme& theme);
    void createDemoContent();
    
    // Clip operations
    void sliceClipAtPosition(PatternClip* clip, double position);
    void makeClipUnique(PatternClip* clip);
    void consolidateSelection();
    void fitClipToTempo(PatternClip* clip, double targetBPM);
    
    double snapToGrid(double beats) const;
};

} // namespace daw::ui::imgui
