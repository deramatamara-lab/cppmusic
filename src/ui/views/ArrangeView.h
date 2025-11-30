#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../lookandfeel/DesignSystem.h"
#include "../../project/ProjectModel.h"
#include "../../project/UndoManager.h"
#include "../../audio/engine/EngineContext.h"
#include "TrackHeaderComponent.h"
#include <memory>
#include <vector>

namespace daw::ui::views
{

/**
 * @brief Arrangement/timeline view
 *
 * Displays timeline with ruler, grid, track rows, and clips.
 * Supports clip selection and dragging.
 * Follows DAW_DEV_RULES: uses design system, responsive layout.
 */
class ArrangeView : public juce::Component, public juce::DragAndDropTarget
{
public:
    ArrangeView(std::shared_ptr<daw::project::ProjectModel> projectModel,
                std::shared_ptr<daw::audio::engine::EngineContext> engineContext,
                daw::project::UndoManager* undoManager = nullptr);
    ~ArrangeView() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

    // Drag and drop
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    void refresh();

    // Snap-to-grid settings
    void setSnapEnabled(bool enabled) { snapEnabled = enabled; }
    void setSnapDivision(double beats) { snapDivision = beats; }
    [[nodiscard]] bool isSnapEnabled() const { return snapEnabled; }

    // Zoom controls
    void setZoom(double pixelsPerBeat);
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    [[nodiscard]] double getZoom() const { return pixelsPerBeat; }

private:
    std::shared_ptr<daw::project::ProjectModel> projectModel;
    std::shared_ptr<daw::audio::engine::EngineContext> engineContext;
    daw::project::UndoManager* undoManager;

    double pixelsPerBeat;
    double minPixelsPerBeat;
    double maxPixelsPerBeat;
    int trackHeight;
    int rulerHeight;

    bool isDragging;
    bool isBoxSelecting;
    bool isTrimming;
    bool trimStart; // true = trimming start (left edge), false = trimming end (right edge)
    uint32_t draggedClipId;
    double dragStartBeats;
    double dragStartLength;
    juce::Point<int> dragStartPos;
    juce::Rectangle<int> boxSelectRect;

    // Enhanced clip editing state
    bool isResizing;
    bool isSettingFade;
    bool resizeFromStart; // true = resizing from start, false = from end
    uint32_t hoveredClipId;
    juce::Point<int> lastMousePos;

    // Resize handle detection
    static constexpr int kResizeHandleWidth = 8;
    static constexpr int kFadeHandleWidth = 20;

    // Snap-to-grid
    bool snapEnabled = true;
    double snapDivision = 0.25; // 1/16th note by default

    // Track headers
    std::vector<std::unique_ptr<TrackHeaderComponent>> trackHeaders;

    // Clipboard for cut/copy/paste operations
    struct ClipboardData
    {
        std::vector<daw::project::Clip> clips;
        double originBeats = 0.0; // Position reference for pasting
        bool isCutOperation = false; // true for cut, false for copy
    };
    ClipboardData clipboard;

    void drawRuler(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawTracks(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawClips(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawContainers(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawBoxSelection(juce::Graphics& g, juce::Rectangle<int> bounds);

    // Enhanced timeline ruler methods
    void drawTimelineSubdivisions(juce::Graphics& g, juce::Rectangle<int> bounds, double beatsPerBar, int subdivLevel);
    void drawTimelineLabels(juce::Graphics& g, juce::Rectangle<int> bounds, double beatsPerBar, int subdivLevel);
    void drawTimeSignatureIndicator(juce::Graphics& g, juce::Rectangle<int> bounds, int numerator, int denominator);
    void drawSubdivisionLines(juce::Graphics& g, juce::Rectangle<int> bounds, double interval, double maxBeats,
                              juce::Colour colour, float lineWidth, float alpha);

    juce::Rectangle<int> getClipBounds(const daw::project::Clip* clip) const;
    daw::project::Clip* getClipAtPosition(juce::Point<int> pos) const;
    std::vector<daw::project::Clip*> getClipsInRect(juce::Rectangle<int> rect) const;
    bool isNearClipEdge(juce::Point<int> pos, const daw::project::Clip* clip, bool& isStartEdge) const;
    double beatsFromX(int x) const;
    int xFromBeats(double beats) const;
    double snapBeats(double beats) const;

    // Track header management
    void updateTrackHeaders();
    void refreshTrackHeaders();

    // Enhanced clip editing methods
    void drawClipResizeHandles(juce::Graphics& g, const daw::project::Clip* clip, juce::Rectangle<int> clipBounds, bool isSelected);
    void drawClipFadeHandles(juce::Graphics& g, const daw::project::Clip* clip, juce::Rectangle<int> clipBounds);
    bool isOverResizeHandle(juce::Point<int> pos, const daw::project::Clip* clip, bool& isStartHandle);
    bool isOverFadeHandle(juce::Point<int> pos, const daw::project::Clip* clip, bool& isStartHandle);
    juce::Colour getClipTypeColour(const daw::project::Clip* clip);
    void showClipContextMenu(const daw::project::Clip* clip, juce::Point<int> position);
    void handleClipContextMenuResult(int result, const daw::project::Clip* clip);

    // Clip operations
    void cutSelectedClips();
    void copySelectedClips();
    void pasteClips();
    void duplicateSelectedClips();
    void splitClipAtPlayhead(const daw::project::Clip* clip);
    void setClipColor(const daw::project::Clip* clip, int colorIndex);
    void deleteSelectedClips();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangeView)
};

} // namespace daw::ui::views

inline bool daw::ui::views::ArrangeView::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0))
    {
        cutSelectedClips();
        return true;
    }

    if (key == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0))
    {
        copySelectedClips();
        return true;
    }

    if (key == juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0))
    {
        pasteClips();
        return true;
    }

    if (key == juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0))
    {
        duplicateSelectedClips();
        return true;
    }

    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        deleteSelectedClips();
        return true;
    }

    return false;
}

