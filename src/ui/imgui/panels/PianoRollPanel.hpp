#pragma once

#include "../Theme.hpp"
#include "../../model/Pattern.hpp"
#include <vector>
#include <functional>
#include <string>

namespace daw::ui::imgui
{

/**
 * @brief Tool mode for piano roll editing
 */
enum class PianoRollTool {
    Draw,           ///< Draw new notes
    Select,         ///< Select and move notes
    Slice,          ///< Slice notes at cursor
    Glue,           ///< Glue adjacent notes
    Stretch,        ///< Time-stretch selection
    Warp,           ///< Insert/edit warp markers
    Erase,          ///< Erase notes
    VelocityPaint,  ///< Paint velocity values
    TimeNudge,      ///< Nudge timing micro-adjustments
    ChordStamp,     ///< FL-style chord stamp tool
    Strum,          ///< Strum selected notes
    Arpeggiate      ///< Arpeggiate chord to sequence
};

/**
 * @brief Chord type for chord stamp tool (FL Studio style)
 */
enum class ChordType {
    Major,
    Minor,
    Diminished,
    Augmented,
    Major7,
    Minor7,
    Dom7,
    Dim7,
    Sus2,
    Sus4,
    Add9,
    Power,
    Custom
};

/**
 * @brief Note color mode for per-note coloring
 */
enum class NoteColorMode {
    Velocity,       ///< Color by velocity
    Channel,        ///< Color by MIDI channel
    Pitch,          ///< Color by pitch/octave
    Custom,         ///< Per-note custom color
    Probability     ///< Color by probability
};

/**
 * @brief Note event for piano roll with deep-edit capabilities
 */
struct NoteEvent
{
    int pitch{60};             // MIDI note number (0-127)
    double startBeats{0.0};
    double lengthBeats{1.0};
    float velocity{0.8f};
    bool selected{false};

    // Deep-edit properties
    float releaseVelocity{0.5f};  ///< Note-off velocity [0, 1]
    float pitchOffset{0.0f};      ///< Per-note pitch offset in semitones

    // Slide/portamento
    bool hasSlide{false};         ///< Enable slide to next note
    float slideTime{0.0f};        ///< Slide duration in beats
    int slideToPitch{0};          ///< Relative pitch target for slide

    // Probability and conditions
    float probability{1.0f};      ///< Play probability [0, 1]
    int condition{0};             ///< 0=always, 1=first, 2=Nth, 3=everyN, 4=skipM, 5=random
    int conditionParam{1};        ///< Parameter for condition

    // Micro-timing
    int microTimingOffset{0};     ///< Sub-tick offset in samples
    float swingAmount{0.0f};      ///< Per-note swing override

    // FL Studio-style per-note color
    ImVec4 customColor{0.3f, 0.5f, 0.8f, 1.0f};  ///< Per-note color
    bool useCustomColor{false};   ///< Use custom color instead of default

    // MIDI channel (FL-style multi-channel editing)
    int midiChannel{0};           ///< MIDI channel (0-15)

    // Expression data
    float pan{0.5f};              ///< Per-note pan [0=L, 1=R]
    float finePitch{0.0f};        ///< Fine pitch adjustment in cents (-100 to +100)
    float modWheel{0.0f};         ///< Mod wheel value at note start
    float filter{1.0f};           ///< Filter cutoff multiplier
};

/**
 * @brief Warp marker for time transformation
 */
struct WarpMarker
{
    double sourceBeat{0.0};
    double targetBeat{0.0};
    bool selected{false};
};

/**
 * @brief Piano Roll panel for MIDI editing
 *
 * Features:
 * - Piano keyboard with note grid
 * - Pan/zoom controls with inertia
 * - Advanced tool modes (Draw, Select, Slice, Glue, Stretch, Warp, Erase, Velocity Paint, Time Nudge)
 * - Velocity & CC lanes with brush/ramp tools
 * - Scale lock and ghost notes
 * - Slide/portamento visualization
 * - Probability and condition indicators
 * - Micro-timing visualization
 * - Warp markers for time transformation
 */
class PianoRollPanel
{
public:
    PianoRollPanel();
    ~PianoRollPanel() = default;

    /**
     * @brief Draw the piano roll panel
     * @param open Reference to visibility flag
     * @param theme Theme for styling
     */
    void draw(bool& open, const Theme& theme);

    /**
     * @brief Get notes
     */
    [[nodiscard]] std::vector<NoteEvent>& getNotes() { return notes_; }

    /**
     * @brief Get warp markers
     */
    [[nodiscard]] std::vector<WarpMarker>& getWarpMarkers() { return warpMarkers_; }

    /**
     * @brief Set callback for note changes
     */
    void setOnNoteChanged(std::function<void(const NoteEvent&)> callback)
    {
        onNoteChanged_ = std::move(callback);
    }

    /**
     * @brief Set the current tool
     */
    void setCurrentTool(PianoRollTool tool) { currentTool_ = tool; }

    /**
     * @brief Get the current tool
     */
    [[nodiscard]] PianoRollTool getCurrentTool() const { return currentTool_; }

    /**
     * @brief Execute command palette action
     */
    void executeCommand(const std::string& command);

private:
    std::vector<NoteEvent> notes_;
    std::vector<WarpMarker> warpMarkers_;

    // View state
    double scrollX_{0.0};
    double scrollY_{60.0};  // Center around middle C
    float zoomX_{1.0f};
    float zoomY_{1.0f};
    float pixelsPerBeat_{40.0f};
    float noteHeight_{12.0f};

    // Zoom/pan inertia
    float scrollVelocityX_{0.0f};
    float scrollVelocityY_{0.0f};
    float zoomVelocity_{0.0f};

    // Tool state
    PianoRollTool currentTool_{PianoRollTool::Draw};
    bool showVelocity_{true};
    bool showProbability_{false};
    bool showMicroTiming_{false};
    int snapDivision_{4};  // 1/4 note

    // Scale lock
    bool scaleLockEnabled_{false};
    int scaleRoot_{0};  // C
    std::vector<bool> scaleNotes_{true, false, true, false, true, true, false, true, false, true, false, true};

    // Ghost notes from other patterns
    bool showGhostNotes_{false};
    std::vector<NoteEvent> ghostNotes_;

    // Fold mode (show only used notes)
    bool foldMode_{false};
    std::vector<int> usedPitches_;

    // Selection state
    std::vector<size_t> selectedNoteIndices_;
    bool isDragging_{false};
    bool isBoxSelecting_{false};
    ImVec2 boxSelectStart_;
    ImVec2 boxSelectEnd_;

    // Drag modifiers state
    bool dragDuplicating_{false};  // Ctrl held during drag
    bool dragFineAdjust_{false};   // Shift held during drag
    bool dragIgnoreSnap_{false};   // Alt held during drag

    // Hover preview
    NoteEvent hoverPreviewNote_;
    bool showHoverPreview_{false};

    // Command palette
    bool showCommandPalette_{false};
    std::string commandInput_;

    // FL Studio-style chord stamp
    ChordType currentChord_{ChordType::Major};
    std::vector<int> customChordIntervals_;
    int chordInversion_{0};       // 0 = root, 1 = 1st inv, 2 = 2nd inv
    float strumDelay_{0.02f};     // Strum delay in beats
    bool strumUp_{false};         // Strum direction

    // Note color mode
    NoteColorMode noteColorMode_{NoteColorMode::Velocity};
    std::vector<ImVec4> channelColors_;  // Colors per MIDI channel

    // FL-style MIDI effects (non-destructive)
    bool midiEchoEnabled_{false};
    int midiEchoCount_{3};
    float midiEchoDelay_{0.25f};  // In beats
    float midiEchoDecay_{0.7f};   // Velocity multiplier per echo

    bool midiArpEnabled_{false};
    float midiArpRate_{0.25f};    // In beats
    int midiArpOctaves_{1};
    bool midiArpPingPong_{false};

    // Piano preview
    bool previewOnClick_{true};
    int previewVelocity_{100};

    // Snap modes
    bool magneticSnap_{true};     // Snap attracts nearby notes
    bool tripletMode_{false};     // Enable triplet grid

    std::function<void(const NoteEvent&)> onNoteChanged_;

    // Drawing methods
    void drawToolbar(const Theme& theme);
    void drawPianoKeys(const Theme& theme);
    void drawGrid(const Theme& theme);
    void drawNotes(const Theme& theme);
    void drawSlideConnections(const Theme& theme);
    void drawVelocityLane(const Theme& theme);
    void drawProbabilityLane(const Theme& theme);
    void drawMicroTimingLane(const Theme& theme);
    void drawPanLane(const Theme& theme);
    void drawPitchLane(const Theme& theme);
    void drawFilterLane(const Theme& theme);
    void drawWarpMarkers(const Theme& theme);
    void drawHoverPreview(const Theme& theme);
    void drawBoxSelection(const Theme& theme);
    void drawCommandPalette(const Theme& theme);
    void drawChordStampPreview(const Theme& theme);
    void drawMIDIEchoPreview(const Theme& theme);

    // Input handling
    void handleInput(const Theme& theme);
    void handleToolInput(const Theme& theme);
    void handleDragModifiers();
    void handleScrubPlayback();
    void handleZoomPan();

    // Note operations
    void createDemoNotes();
    void selectAll();
    void selectNone();
    void deleteSelected();
    void duplicateSelected();
    void quantizeSelected();
    void legato();
    void randomizeSelection(bool velocity, bool timing);
    void makeUnique();

    // FL Studio-style operations
    void stampChord(double beat, int rootPitch);
    void strumSelection(float delay, bool upward);
    void arpeggiateSelection(float rate, int octaves, bool pingPong);
    void humanizeSelection(float timingAmount, float velocityAmount);
    void flipVertical();
    void flipHorizontal();
    void scaleVelocities(float factor);
    void limitNotes(int maxPolyphony);
    void chokeGroup();
    void quickLegato();
    void quickQuantize();
    void applyMIDIEcho();
    void consolidateNotes();
    void splitByChannel();
    std::vector<int> getChordIntervals(ChordType type) const;

    // Utility methods
    bool isNoteInScale(int pitch) const;
    double snapToGrid(double beats) const;
    int yToPitch(float y) const;
    float pitchToY(int pitch) const;
    double xToBeats(float x) const;
    float beatsToX(double beats) const;
    void updateUsedPitches();
};

} // namespace daw::ui::imgui
