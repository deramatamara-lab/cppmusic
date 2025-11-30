#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"
#include "../../project/ProjectModel.h"
#include "../../ai/inference/InferenceEngine.h"
#include <memory>

namespace daw::ui::views
{

/**
 * @brief Advanced piano roll editor
 *
 * World-class piano roll with MPE support, quantization, ghost notes,
 * and advanced editing tools. Follows DAW_DEV_RULES: professional UX.
 */
class PianoRollView : public juce::Component,
                      public juce::Timer
{
public:
    PianoRollView();
    ~PianoRollView() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    /**
     * @brief Set project model
     */
    void setProjectModel(std::shared_ptr<daw::project::ProjectModel> model);

    /**
     * @brief Set current track for editing
     */
    void setCurrentTrack(uint32_t trackId);

    /**
     * @brief Set current clip for editing (loads pattern if clip has one)
     */
    void setCurrentClip(uint32_t clipId);

    /**
     * @brief Set quantization grid
     */
    void setQuantization(double gridDivision);

    /**
     * @brief Enable/disable scale snapping
     */
    void setScaleSnapping(bool enabled, const std::vector<int>& scaleIntervals);

    /**
     * @brief Show ghost notes from other tracks
     */
    void setShowGhostNotes(bool show, uint32_t referenceTrackId);

    /**
     * @brief Set AI inference engine
     */
    void setInferenceEngine(std::shared_ptr<daw::ai::inference::InferenceEngine> engine);

private:
    void timerCallback() override;

    struct NoteRect
    {
        int note;
        double startBeat;
        double lengthBeats;
        juce::Rectangle<float> bounds;
        bool isSelected{false};
    };

    std::shared_ptr<daw::project::ProjectModel> projectModel;
    uint32_t currentTrackId{0};
    uint32_t currentClipId{0};
    uint32_t currentPatternId{0};

    double quantization{1.0 / 16.0};
    bool scaleSnapping{false};
    std::vector<int> scaleIntervals;
    bool showGhostNotes{false};
    uint32_t ghostTrackId{0};

    double viewStartBeat{0.0};
    double viewEndBeat{16.0};
    int viewStartNote{0};
    int viewEndNote{128};
    float pixelsPerBeat{50.0f};
    float pixelsPerNote{12.0f};

    std::vector<NoteRect> notes;
    std::vector<NoteRect> ghostNotes;

    int hoveredNote{-1};
    int draggedNoteIndex{-1};
    bool isDragging{false};
    bool isCreatingNote{false};
    juce::Point<int> dragStart;
    daw::project::Pattern::MIDINote originalNote; // For tracking note updates

    void updateNotes();
    void updateGhostNotes();
    [[nodiscard]] juce::Rectangle<float> noteToRect(int note, double startBeat, double lengthBeats) const;
    [[nodiscard]] std::pair<int, double> rectToNote(const juce::Point<int>& point) const;
    [[nodiscard]] int snapNoteToScale(int note) const;
    [[nodiscard]] double snapBeatToGrid(double beat) const;
    void repaintNotes();

    // AI buttons
    juce::TextButton aiGenerateMelodyButton;
    juce::TextButton aiGenerateChordsButton;
    juce::TextButton aiGenerateGrooveButton;
    std::shared_ptr<daw::ai::inference::InferenceEngine> inferenceEngine;
    bool isAIGenerating{false};

    void setupAIButtons();
    void aiGenerateMelodyClicked();
    void aiGenerateChordsClicked();
    void aiGenerateGrooveClicked();

    // Parse AI-generated MIDI notes from JSON response
    bool parseAIMIDINotes(const std::string& jsonResponse);
};

} // namespace daw::ui::views

