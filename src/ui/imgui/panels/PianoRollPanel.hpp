#pragma once

#include "../Theme.hpp"
#include <vector>
#include <functional>

namespace daw::ui::imgui
{

/**
 * @brief Note event for piano roll
 */
struct NoteEvent
{
    int pitch{60};           // MIDI note number (0-127)
    double startBeats{0.0};
    double lengthBeats{1.0};
    float velocity{0.8f};
    bool selected{false};
};

/**
 * @brief Piano Roll panel for MIDI editing
 * 
 * Features:
 * - Piano keyboard with note grid
 * - Pan/zoom controls
 * - Draw/edit/select tools
 * - Velocity & CC lanes
 * - Scale lock buttons
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
     * @brief Set callback for note changes
     */
    void setOnNoteChanged(std::function<void(const NoteEvent&)> callback)
    {
        onNoteChanged_ = std::move(callback);
    }

private:
    std::vector<NoteEvent> notes_;
    
    // View state
    double scrollX_{0.0};
    double scrollY_{60.0};  // Center around middle C
    float zoomX_{1.0f};
    float zoomY_{1.0f};
    float pixelsPerBeat_{40.0f};
    float noteHeight_{12.0f};
    
    // Tool state
    enum class Tool { Draw, Select, Erase };
    Tool currentTool_{Tool::Draw};
    bool showVelocity_{true};
    int snapDivision_{4};  // 1/4 note
    
    // Scale lock
    bool scaleLockEnabled_{false};
    int scaleRoot_{0};  // C
    std::vector<bool> scaleNotes_{true, false, true, false, true, true, false, true, false, true, false, true};
    
    std::function<void(const NoteEvent&)> onNoteChanged_;

    void drawToolbar(const Theme& theme);
    void drawPianoKeys(const Theme& theme);
    void drawGrid(const Theme& theme);
    void drawNotes(const Theme& theme);
    void drawVelocityLane(const Theme& theme);
    void createDemoNotes();
    
    bool isNoteInScale(int pitch) const;
    double snapToGrid(double beats) const;
    int yToPitch(float y) const;
    float pitchToY(int pitch) const;
    double xToBeats(float x) const;
    float beatsToX(double beats) const;
};

} // namespace daw::ui::imgui
