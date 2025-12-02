#pragma once

#include "../Theme.hpp"
#include <vector>
#include <string>
#include <functional>

namespace daw::ui::imgui
{

/**
 * @brief Marker type for timeline markers
 */
enum class MarkerType {
    Generic,
    LoopStart,
    LoopEnd,
    PunchIn,
    PunchOut,
    TimeSignature,
    Tempo,
    Section
};

/**
 * @brief Timeline marker (FL Studio style)
 */
struct TimelineMarker
{
    double position{0.0};     ///< Position in beats
    std::string name;
    MarkerType type{MarkerType::Generic};
    ImVec4 color{1.0f, 0.5f, 0.0f, 1.0f};

    // For time signature markers
    int numerator{4};
    int denominator{4};

    // For tempo markers
    double tempo{120.0};
};

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
 * @brief Automation point for automation clips
 */
struct AutomationPoint
{
    double beat{0.0};
    float value{0.5f};
    int curveType{0};  // 0=linear, 1=smooth, 2=step, 3=pulse
    float tension{0.0f};
    bool selected{false};
};

/**
 * @brief Clip type enumeration
 */
enum class ClipType {
    Pattern,
    Audio,
    Automation
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

    // Clip type
    ClipType type{ClipType::Pattern};

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

    // Audio clip specific
    std::string audioFilePath;
    double originalBPM{120.0};
    bool reversePlayback{false};
    float fadeInBeats{0.0f};
    float fadeOutBeats{0.0f};

    // Automation clip specific
    std::vector<AutomationPoint> automationPoints;
    int targetParameter{-1};
    std::string targetParameterName;
    float minValue{0.0f};
    float maxValue{1.0f};

    // Pattern reference
    int patternIndex{0};             ///< Index into pattern bank
};

/**
 * @brief Track state in playlist
 */
struct PlaylistTrack
{
    std::string name;
    ImVec4 color{0.5f, 0.5f, 0.5f, 1.0f};
    float height{60.0f};
    bool muted{false};
    bool soloed{false};
    bool locked{false};
    bool collapsed{false};
    int groupId{-1};  // Parent group track (-1 = no parent)
    bool isGroup{false};
};

/**
 * @brief Playlist tool modes
 */
enum class PlaylistTool {
    Select,     ///< Select and move clips
    Slice,      ///< Slice clips
    Slip,       ///< Slip edit (adjust content offset)
    Stretch,    ///< Time-stretch clips
    Draw,       ///< Draw new clips
    Erase,      ///< Erase clips
    Mute,       ///< Toggle mute on clips
    Playback,   ///< Scrub playback
    Zoom        ///< Zoom tool
};

/**
 * @brief Playlist panel for arrangement view (FL Studio style)
 *
 * Features:
 * - Timeline with pattern/audio/automation clips
 * - Track lanes with headers and groups
 * - Snapping and selection marquee
 * - Clip warp/stretch with RubberBand-style interface
 * - Slip editing
 * - Per-instance transpose/gain
 * - Mute and ghost states
 * - Loop region and smooth playhead follow
 * - Timeline markers (sections, tempo, time signature)
 * - Track groups and folder tracks
 * - Automation clip editing
 * - Pattern picker panel
 * - Consolidate/bounce selection
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
     * @brief Get tracks
     */
    [[nodiscard]] std::vector<PlaylistTrack>& getTracks() { return tracks_; }

    /**
     * @brief Get markers
     */
    [[nodiscard]] std::vector<TimelineMarker>& getMarkers() { return markers_; }

    /**
     * @brief Add a clip
     */
    void addClip(const PatternClip& clip);

    /**
     * @brief Add a track
     */
    void addTrack(const PlaylistTrack& track);

    /**
     * @brief Add a marker
     */
    void addMarker(const TimelineMarker& marker);

    /**
     * @brief Set callback for clip selection
     */
    void setOnClipSelected(std::function<void(PatternClip*)> callback)
    {
        onClipSelected_ = std::move(callback);
    }

    /**
     * @brief Set callback for pattern double-click (opens piano roll)
     */
    void setOnClipDoubleClick(std::function<void(PatternClip*)> callback)
    {
        onClipDoubleClick_ = std::move(callback);
    }

    /**
     * @brief Set the current tool
     */
    void setCurrentTool(PlaylistTool tool) { currentTool_ = tool; }

private:
    std::vector<PatternClip> clips_;
    std::vector<PlaylistTrack> tracks_;
    std::vector<TimelineMarker> markers_;
    std::vector<std::string> patternBank_;  // Available patterns

    // View state
    double scrollX_{0.0};
    double scrollY_{0.0};
    float zoomX_{1.0f};
    float zoomY_{1.0f};
    float pixelsPerBeat_{20.0f};
    float defaultTrackHeight_{60.0f};

    // Selection
    PatternClip* selectedClip_{nullptr};
    std::vector<PatternClip*> multiSelection_;
    bool isDragging_{false};
    bool isSelecting_{false};
    bool isSlipEditing_{false};
    bool isStretching_{false};
    bool isResizing_{false};
    ImVec2 selectionStart_;
    ImVec2 selectionEnd_;

    // Tool state
    PlaylistTool currentTool_{PlaylistTool::Select};

    // Snapping
    bool snapEnabled_{true};
    int snapDivision_{4};
    bool magneticSnap_{true};

    // Loop region
    bool loopEnabled_{false};
    double loopStart_{0.0};
    double loopEnd_{16.0};

    // Playhead
    double playheadPosition_{0.0};
    bool playheadFollowing_{true};

    // Pattern picker
    bool showPatternPicker_{false};
    int selectedPatternIndex_{0};

    // Automation editing
    bool editingAutomation_{false};
    int selectedAutomationPoint_{-1};

    // View options
    bool showTrackHeaders_{true};
    bool showMinimap_{false};
    bool showMarkers_{true};
    bool highlightCurrentBar_{true};
    bool showGridLabels_{true};

    std::function<void(PatternClip*)> onClipSelected_;
    std::function<void(PatternClip*)> onClipDoubleClick_;

    void drawToolbar(const Theme& theme);
    void drawTimeline(const Theme& theme);
    void drawTrackHeaders(const Theme& theme);
    void drawTracks(const Theme& theme);
    void drawClips(const Theme& theme);
    void drawPatternClip(PatternClip& clip, const Theme& theme, ImVec2 clipPos, ImVec2 clipSize);
    void drawAudioClip(PatternClip& clip, const Theme& theme, ImVec2 clipPos, ImVec2 clipSize);
    void drawAutomationClip(PatternClip& clip, const Theme& theme, ImVec2 clipPos, ImVec2 clipSize);
    void drawClipWarpMarkers(const PatternClip& clip, const Theme& theme, ImVec2 clipPos, ImVec2 clipSize);
    void drawSelectionMarquee(const Theme& theme);
    void drawPlayhead(const Theme& theme);
    void drawLoopRegion(const Theme& theme);
    void drawMarkers(const Theme& theme);
    void drawPatternPicker(const Theme& theme);
    void drawMinimap(const Theme& theme);
    void createDemoContent();

    // Clip operations
    void sliceClipAtPosition(PatternClip* clip, double position);
    void makeClipUnique(PatternClip* clip);
    void consolidateSelection();
    void fitClipToTempo(PatternClip* clip, double targetBPM);
    void ghostSelection();
    void unghost(PatternClip* clip);
    void mergeSelectedClips();
    void splitClipsByTrack();

    // Track operations
    void groupSelectedTracks();
    void ungroupTrack(int trackIndex);
    void collapseTrackGroup(int groupIndex);
    void expandTrackGroup(int groupIndex);

    double snapToGrid(double beats) const;
    int getTrackAtY(float y) const;
    double getBeatsAtX(float x) const;
};

} // namespace daw::ui::imgui
