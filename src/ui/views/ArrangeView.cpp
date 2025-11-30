#include "ArrangeView.h"
#include "../lookandfeel/DesignSystem.h"
#include "../../project/commands/MoveClipCommand.h"
#include "../../project/commands/RemoveClipCommand.h"
#include "../../project/commands/TrimClipCommand.h"
#include <limits>
#include <algorithm>

namespace daw::ui::views
{

ArrangeView::ArrangeView(std::shared_ptr<daw::project::ProjectModel> projectModel,
                         std::shared_ptr<daw::audio::engine::EngineContext> engineContext,
                         daw::project::UndoManager* undoManager)
    : projectModel(projectModel)
    , engineContext(engineContext)
    , undoManager(undoManager)
    , pixelsPerBeat(daw::ui::lookandfeel::DesignSystem::Layout::kPixelsPerBeat)
    , minPixelsPerBeat(10.0)
    , maxPixelsPerBeat(500.0)
    , trackHeight(daw::ui::lookandfeel::DesignSystem::Layout::kTrackHeight)
    , rulerHeight(daw::ui::lookandfeel::DesignSystem::Layout::kTimelineRulerHeight)
    , isDragging(false)
    , isBoxSelecting(false)
    , isTrimming(false)
    , trimStart(false)
    , draggedClipId(0)
    , dragStartBeats(0.0)
    , dragStartLength(0.0)
{
    updateTrackHeaders();
}

void ArrangeView::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    g.fillAll(juce::Colour(Colors::background));

    auto bounds = getLocalBounds();

    // Draw ruler with glassmorphism
    auto rulerBounds = bounds.removeFromTop(rulerHeight);
    drawRuler(g, rulerBounds);

    // Draw grid and tracks
    drawGrid(g, bounds);
    drawTracks(g, bounds);
    drawContainers(g, bounds); // Draw container backgrounds first
    drawClips(g, bounds);
    drawBoxSelection(g, bounds);
}

void ArrangeView::resized()
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    auto bounds = getLocalBounds();

    // Timeline ruler at top
    bounds.removeFromTop(rulerHeight);

    // Position track headers
    auto headerArea = bounds.removeFromLeft(Layout::kTrackHeaderWidth);

    int trackIndex = 0;
    for (auto& header : trackHeaders)
    {
        auto trackBounds = headerArea.removeFromTop(trackHeight);
        header->setBounds(trackBounds);
        ++trackIndex;
    }

    refresh();
}

void ArrangeView::mouseDown(const juce::MouseEvent& e)
{
    if (projectModel == nullptr)
        return;

    dragStartPos = e.position.toInt();

    // Check if clicking on a clip
    auto clip = getClipAtPosition(e.position.toInt());
    
    if (clip != nullptr)
    {
        auto& selection = projectModel->getSelectionModel();
        const auto isSelected = selection.isClipSelected(clip->getId());
        
        // First check for resize handles on selected clips
        if (isSelected && isOverResizeHandle(e.position.toInt(), clip, resizeFromStart))
        {
            isResizing = true;
            draggedClipId = clip->getId();
            dragStartBeats = clip->getStartBeats();
            dragStartLength = clip->getLengthBeats();
            return;
        }
        
        // Check for fade handles on selected clips
        bool fadeIn = false;
        if (isSelected && isOverFadeHandle(e.position.toInt(), clip, fadeIn))
        {
            isSettingFade = true;
            resizeFromStart = fadeIn; // Reuse flag for fade direction
            draggedClipId = clip->getId();
            return;
        }

        // Check if clicking near edge for legacy trimming (only if single clip selected)
        bool nearEdge = false;
        bool isStartEdge = false;
        if (clip != nullptr)
        {
            nearEdge = isNearClipEdge(e.position.toInt(), clip, isStartEdge);
        }
        
        if (nearEdge && isSelected && selection.getSelectedClips().size() == 1)
        {
            isTrimming = true;
            trimStart = isStartEdge;
            draggedClipId = clip->getId();
            dragStartBeats = clip->getStartBeats();
            dragStartLength = clip->getLengthBeats();
            return;
        }

        // Handle right-click context menu
        if (e.mods.isPopupMenu())
        {
            if (!isSelected)
            {
                selection.clearAll();
                selection.selectClip(clip->getId());
            }
            showClipContextMenu(clip, e.position.toInt());
            return;
        }

        // Multi-selection: Ctrl/Cmd adds to selection, Shift adds range
        if (e.mods.isCommandDown() || e.mods.isCtrlDown())
        {
            if (isSelected)
                selection.deselectClip(clip->getId());
            else
                selection.selectClip(clip->getId());
        }
        else if (e.mods.isShiftDown())
        {
            // Range selection: select from last selected to this clip
            const auto selectedClips = selection.getSelectedClips();
            if (!selectedClips.empty())
            {
                // TODO: Implement range selection logic
                selection.selectClip(clip->getId());
            }
            else
            {
                selection.selectClip(clip->getId());
            }
        }
        else
        {
            // Single selection
            if (!isSelected)
            {
                selection.clearAll();
                selection.selectClip(clip->getId());
            }
        }

        // Start dragging if selected (and not resizing/trimming)
        if (selection.isClipSelected(clip->getId()) && !nearEdge && !isResizing && !isSettingFade)
        {
            isDragging = true;
            draggedClipId = clip->getId();
            dragStartBeats = clip->getStartBeats();
        }
    }
    else
    {
        // Start box selection if not clicking on clip and not in ruler
        if (e.position.y > rulerHeight)
        {
            isBoxSelecting = true;
            boxSelectRect = juce::Rectangle<int>(dragStartPos, dragStartPos);

            // Clear selection if not holding Ctrl/Cmd
            if (!e.mods.isCommandDown() && !e.mods.isCtrlDown())
            {
                projectModel->getSelectionModel().clearAll();
            }
        }
        else
        {
            // Click in ruler - deselect all
            projectModel->getSelectionModel().clearAll();
        }
    }

    repaint();
}

void ArrangeView::mouseDrag(const juce::MouseEvent& e)
{
    if (projectModel == nullptr)
        return;

    if (isResizing && draggedClipId != 0)
    {
        // Resize clip with handles
        auto* clip = projectModel->getClip(draggedClipId);
        if (clip != nullptr)
        {
            const auto deltaX = e.position.toInt().x - dragStartPos.x;
            auto deltaBeats = beatsFromX(deltaX);

            if (resizeFromStart)
            {
                // Resizing from start (left handle)
                auto newStart = dragStartBeats + deltaBeats;
                auto newLength = dragStartLength - deltaBeats;

                if (snapEnabled)
                {
                    newStart = snapBeats(newStart);
                    newLength = dragStartLength - (newStart - dragStartBeats);
                }

                if (newLength > 0.01 && newStart >= 0.0)
                {
                    clip->setStartBeats(newStart);
                    clip->setLengthBeats(newLength);
                }
            }
            else
            {
                // Resizing from end (right handle)
                auto newLength = dragStartLength + deltaBeats;

                if (snapEnabled)
                {
                    const auto endBeats = dragStartBeats + newLength;
                    const auto snappedEnd = snapBeats(endBeats);
                    newLength = snappedEnd - dragStartBeats;
                }

                if (newLength > 0.01)
                {
                    clip->setLengthBeats(newLength);
                }
            }

            repaint();
        }
    }
    else if (isSettingFade && draggedClipId != 0)
    {
        // Set fade with handles
        auto* clip = projectModel->getClip(draggedClipId);
        if (clip != nullptr)
        {
            const auto deltaX = e.position.toInt().x - dragStartPos.x;
            auto deltaBeats = beatsFromX(deltaX);
            
            // Clamp fade to reasonable values (0.01 to half clip length)
            const auto maxFade = clip->getLengthBeats() * 0.5;
            deltaBeats = juce::jlimit(-maxFade, maxFade, deltaBeats);
            
            if (resizeFromStart) // Using flag for fade direction
            {
                // Setting fade in
                auto fadeIn = clip->getFadeInBeats() + deltaBeats;
                fadeIn = juce::jmax(0.0, juce::jmin(maxFade, fadeIn));
                clip->setFadeInBeats(fadeIn);
            }
            else
            {
                // Setting fade out
                auto fadeOut = clip->getFadeOutBeats() + deltaBeats;
                fadeOut = juce::jmax(0.0, juce::jmin(maxFade, fadeOut));
                clip->setFadeOutBeats(fadeOut);
            }

            repaint();
        }
    }
    else if (isTrimming && draggedClipId != 0)
    {
        // Legacy trim clip
        auto* clip = projectModel->getClip(draggedClipId);
        if (clip != nullptr)
        {
            const auto deltaX = e.position.toInt().x - dragStartPos.x;
            auto deltaBeats = beatsFromX(deltaX);

            if (trimStart)
            {
                // Trimming start (left edge)
                auto newStart = dragStartBeats + deltaBeats;
                auto newLength = dragStartLength - deltaBeats;

                if (snapEnabled)
                {
                    newStart = snapBeats(newStart);
                    newLength = dragStartLength - (newStart - dragStartBeats);
                }

                if (newLength > 0.01 && newStart >= 0.0)
                {
                    clip->setStartBeats(newStart);
                    clip->setLengthBeats(newLength);
                }
            }
            else
            {
                // Trimming end (right edge)
                auto newLength = dragStartLength + deltaBeats;

                if (snapEnabled)
                {
                    const auto endBeats = dragStartBeats + newLength;
                    const auto snappedEnd = snapBeats(endBeats);
                    newLength = snappedEnd - dragStartBeats;
                }

                if (newLength > 0.01)
                {
                    clip->setLengthBeats(newLength);
                }
            }

            repaint();
        }
    }
    else if (isDragging && draggedClipId != 0)
    {
        // Drag selected clips
        const auto& selection = projectModel->getSelectionModel();
        const auto selectedClips = selection.getSelectedClips();

        const auto deltaX = e.position.toInt().x - dragStartPos.x;
        auto deltaBeats = beatsFromX(deltaX);

        // Apply snap-to-grid
        if (snapEnabled)
        {
            const auto snappedStart = snapBeats(dragStartBeats + deltaBeats);
            deltaBeats = snappedStart - dragStartBeats;
        }

        // Move all selected clips
        for (const auto clipId : selectedClips)
        {
            auto* clip = projectModel->getClip(clipId);
            if (clip != nullptr)
            {
                const auto originalStart = (clipId == draggedClipId) ? dragStartBeats : clip->getStartBeats();
                clip->setStartBeats(originalStart + deltaBeats);
            }
        }

        repaint();
    }
    else if (isBoxSelecting)
    {
        // Update box selection rectangle
        boxSelectRect = juce::Rectangle<int>(dragStartPos, e.position.toInt());
        boxSelectRect = boxSelectRect.withY(juce::jmax(rulerHeight, boxSelectRect.getY()));

        // Select clips in box
        const auto clipsInBox = getClipsInRect(boxSelectRect);
        auto& selection = projectModel->getSelectionModel();

        // Add to selection if Ctrl/Cmd is held
        if (!e.mods.isCommandDown() && !e.mods.isCtrlDown())
        {
            selection.clearClipSelection();
        }

        for (auto* clip : clipsInBox)
        {
            selection.selectClip(clip->getId());
        }

        // Repaint only box selection area
        repaint(boxSelectRect.expanded(2));
    }
}

void ArrangeView::mouseUp(const juce::MouseEvent& /*e*/)
{
    if (isResizing && draggedClipId != 0 && undoManager != nullptr && projectModel != nullptr)
    {
        // Create resize command for undo/redo
        auto* clip = projectModel->getClip(draggedClipId);
        if (clip != nullptr)
        {
            const auto oldLength = dragStartLength;
            const auto newLength = clip->getLengthBeats();

            if (std::abs(oldLength - newLength) > 0.001) // Only create command if actually changed
            {
                auto command = std::make_unique<daw::project::TrimClipCommand>(
                    draggedClipId, oldLength, newLength, resizeFromStart);
                undoManager->executeCommand(std::move(command), *projectModel);
            }
        }
    }
    else if (isSettingFade && draggedClipId != 0 && undoManager != nullptr && projectModel != nullptr)
    {
        // Create fade command for undo/redo
        auto* clip = projectModel->getClip(draggedClipId);
        if (clip != nullptr)
        {
            // Note: We could create specific fade commands here, but for now
            // we'll just let the changes persist. More sophisticated undo/redo
            // for fades could be added later.
        }
    }
    else if (isTrimming && draggedClipId != 0 && undoManager != nullptr && projectModel != nullptr)
    {
        // Create trim command for undo/redo
        auto* clip = projectModel->getClip(draggedClipId);
        if (clip != nullptr)
        {
            const auto oldLength = dragStartLength;
            const auto newLength = clip->getLengthBeats();

            if (std::abs(oldLength - newLength) > 0.001) // Only create command if actually changed
            {
                auto command = std::make_unique<daw::project::TrimClipCommand>(
                    draggedClipId, oldLength, newLength, trimStart);
                undoManager->executeCommand(std::move(command), *projectModel);
            }
        }
    }
    else if (isDragging && draggedClipId != 0 && undoManager != nullptr && projectModel != nullptr)
    {
        // Create move command for undo/redo
        auto* clip = projectModel->getClip(draggedClipId);
        if (clip != nullptr)
        {
            const auto oldStart = dragStartBeats;
            const auto newStart = clip->getStartBeats();

            if (std::abs(oldStart - newStart) > 0.001) // Only create command if actually moved
            {
                auto command = std::make_unique<daw::project::MoveClipCommand>(
                    draggedClipId, oldStart, newStart);
                undoManager->executeCommand(std::move(command), *projectModel);
            }
        }
    }

    // Reset all editing states
    isDragging = false;
    isTrimming = false;
    isBoxSelecting = false;
    isResizing = false;
    isSettingFade = false;
    resizeFromStart = false;
    hoveredClipId = 0;
    boxSelectRect = {};
    repaint();
}

void ArrangeView::refresh()
{
    // Check if track count has changed and update headers if needed
    if (projectModel)
    {
        const auto tracks = projectModel->getTracks();
        size_t visibleTrackCount = 0;
        for (auto* track : tracks)
        {
            if (track->isVisible())
                ++visibleTrackCount;
        }

        if (visibleTrackCount != trackHeaders.size())
        {
            updateTrackHeaders();
            return; // updateTrackHeaders calls resized which calls refresh
        }
    }

    refreshTrackHeaders();
    repaint();
}

void ArrangeView::setZoom(double newPixelsPerBeat)
{
    pixelsPerBeat = juce::jlimit(minPixelsPerBeat, maxPixelsPerBeat, newPixelsPerBeat);
    refresh();
}

void ArrangeView::zoomIn()
{
    setZoom(pixelsPerBeat * 1.2);
}

void ArrangeView::zoomOut()
{
    setZoom(pixelsPerBeat / 1.2);
}

void ArrangeView::zoomToFit()
{
    if (projectModel == nullptr)
        return;

    const auto clips = projectModel->getClips();
    if (clips.empty())
    {
        pixelsPerBeat = 50.0;
        refresh();
        return;
    }

    double maxEnd = 0.0;
    for (const auto* clip : clips)
    {
        maxEnd = juce::jmax(maxEnd, clip->getEndBeats());
    }

    const auto availableWidth = getWidth() - 120; // Account for track header
    if (availableWidth > 0 && maxEnd > 0.0)
    {
        pixelsPerBeat = availableWidth / maxEnd;
        pixelsPerBeat = juce::jlimit(minPixelsPerBeat, maxPixelsPerBeat, pixelsPerBeat);
    }

    refresh();
}

void ArrangeView::drawRuler(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Enhanced glassmorphism background for ruler
    auto rulerBounds = bounds.toFloat();
    drawGlassPanel(g, rulerBounds, Radii::none, false);

    // Get current time signature from engine context
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    if (engineContext)
    {
        timeSigNumerator = engineContext->getTimeSignatureNumerator();
        timeSigDenominator = engineContext->getTimeSignatureDenominator();
    }

    // Calculate beats per bar based on time signature
    const double beatsPerBar = static_cast<double>(timeSigNumerator);

    // Determine subdivision level based on zoom
    // 0=Bars, 1=Beats, 2=Sixteenths, 3=ThirtySeconds
    int subdivLevel;
    
    if (pixelsPerBeat >= 100.0)
        subdivLevel = 3; // ThirtySeconds
    else if (pixelsPerBeat >= 40.0)
        subdivLevel = 2; // Sixteenths
    else if (pixelsPerBeat >= 15.0)
        subdivLevel = 1; // Beats
    else
        subdivLevel = 0; // Bars

    // Draw subdivisions
    drawTimelineSubdivisions(g, bounds, beatsPerBar, subdivLevel);

    // Draw bar/beat labels
    drawTimelineLabels(g, bounds, beatsPerBar, subdivLevel);

    // Draw time signature indicator
    drawTimeSignatureIndicator(g, bounds, timeSigNumerator, timeSigDenominator);
}

void ArrangeView::drawTimelineSubdivisions(juce::Graphics& g,
                                           juce::Rectangle<int> bounds,
                                           double beatsPerBar,
                                           SubdivisionLevel level)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    const int viewWidth = bounds.getWidth();
    const double maxBeats = beatsFromX(viewWidth);

    // Define subdivision intervals
    double primaryInterval = beatsPerBar; // Bars
    double secondaryInterval = 1.0; // Beats
    double tertiaryInterval = 0.25; // Sixteenths
    double quaternaryInterval = 0.125; // 32nds

    // Draw subdivisions based on level
    switch (level)
    {
        case 3: // ThirtySeconds
            // Draw 32nd note subdivisions
            drawSubdivisionLines(g, bounds, quaternaryInterval, maxBeats,
                                juce::Colour(Colors::divider).withAlpha(0.2f), 1.0f, 0.2f);
            [[fallthrough]];

        case 2: // Sixteenths
            // Draw 16th note subdivisions
            drawSubdivisionLines(g, bounds, tertiaryInterval, maxBeats,
                                juce::Colour(Colors::divider).withAlpha(0.4f), 1.0f, 0.4f);
            [[fallthrough]];

        case 1: // Beats
            // Draw beat subdivisions
            drawSubdivisionLines(g, bounds, secondaryInterval, maxBeats,
                                juce::Colour(Colors::divider).withAlpha(0.6f), 1.5f, 0.7f);
            [[fallthrough]];

        case 0: // Bars
            // Draw bar subdivisions (always visible)
            drawSubdivisionLines(g, bounds, primaryInterval, maxBeats,
                                juce::Colour(Colors::divider), 2.0f, 1.0f);
            break;
    }
}

void ArrangeView::drawTimelineLabels(juce::Graphics& g, juce::Rectangle<int> bounds, double beatsPerBar, int level)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    const auto font = getMonoFont(Typography::caption);
    const auto smallFont = getMonoFont(Typography::caption);
    g.setFont(font);

    const int viewWidth = bounds.getWidth();
    const double maxBeats = beatsFromX(viewWidth);
    const int numBars = static_cast<int>(maxBeats / beatsPerBar) + 1;

    // Draw bar numbers
    for (int bar = 0; bar < numBars; ++bar)
    {
        const double barBeat = bar * beatsPerBar;
        const int x = xFromBeats(barBeat);

        if (x >= bounds.getX() - 50 && x <= bounds.getRight() + 50)
        {
            // Bar number
            const auto textBounds = juce::Rectangle<float>(static_cast<float>(x + 2),
                                                           static_cast<float>(bounds.getY()),
                                                           40.0f,
                                                           static_cast<float>(bounds.getHeight() * 0.6f));

            drawTextWithShadow(g, juce::String(bar + 1), textBounds, juce::Justification::centredLeft,
                              font, juce::Colour(Colors::text), 1.0f, 0.3f);

            // Beat numbers within bar (for beat and higher subdivision levels)
            if (level >= 1 && pixelsPerBeat >= 25.0) // 1 = Beats level
            {
                g.setFont(smallFont);
                for (int beat = 1; beat < static_cast<int>(beatsPerBar); ++beat)
                {
                    const double beatPosition = barBeat + beat;
                    const int beatX = xFromBeats(beatPosition);

                    if (beatX >= bounds.getX() && beatX <= bounds.getRight())
                    {
                        const auto beatTextBounds = juce::Rectangle<float>(static_cast<float>(beatX + 1),
                                                                          static_cast<float>(bounds.getY() + bounds.getHeight() * 0.5f),
                                                                          20.0f,
                                                                          static_cast<float>(bounds.getHeight() * 0.4f));

                        g.setColour(juce::Colour(Colors::textSecondary));
                        g.drawText(juce::String(beat + 1), beatTextBounds, juce::Justification::centredLeft);
                    }
                }
                g.setFont(font);
            }
        }
    }
}

void ArrangeView::drawTimeSignatureIndicator(juce::Graphics& g, juce::Rectangle<int> bounds, int numerator, int denominator)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Draw time signature in top-right corner of ruler
    const auto timeSigBounds = bounds.removeFromRight(60).reduced(4);

    // Background panel
    drawGlassPanel(g, timeSigBounds.toFloat(), Radii::small, true);

    // Time signature text
    const auto font = getBodyFont(Typography::bodySmall);
    const juce::String timeSigText = juce::String(numerator) + "/" + juce::String(denominator);

    drawTextWithShadow(g, timeSigText, timeSigBounds.toFloat(), juce::Justification::centred,
                      font, juce::Colour(Colors::text), 1.0f, 0.3f);
}

void ArrangeView::drawSubdivisionLines(juce::Graphics& g, juce::Rectangle<int> bounds, double interval, double maxBeats,
                                       juce::Colour colour, float lineWidth, float alpha)
{
    const int numLines = static_cast<int>(maxBeats / interval) + 1;

    for (int i = 0; i < numLines; ++i)
    {
        const double beat = i * interval;
        const int x = xFromBeats(beat);

        if (x >= bounds.getX() - 2 && x <= bounds.getRight() + 2)
        {
            // Create gradient for depth effect
            juce::ColourGradient gradient(colour.withAlpha(alpha * 0.3f),
                                         static_cast<float>(x),
                                         static_cast<float>(bounds.getY()),
                                         colour.withAlpha(alpha),
                                         static_cast<float>(x),
                                         static_cast<float>(bounds.getBottom()),
                                         false);

            g.setGradientFill(gradient);

            // Draw line with specified width
            if (lineWidth <= 1.0f)
            {
                g.drawVerticalLine(x, static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
            }
            else
            {
                g.fillRect(static_cast<float>(x - lineWidth/2), static_cast<float>(bounds.getY()),
                          lineWidth, static_cast<float>(bounds.getHeight()));
            }
        }
    }
}

void ArrangeView::drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    const auto totalBeats = static_cast<int>((bounds.getWidth() / pixelsPerBeat)) + 1;
    const int beatsPerBar = 4;

    // Alternating row backgrounds for track lanes
    if (projectModel != nullptr)
    {
        const auto tracks = projectModel->getTracks();
        int laneIndex = 0;
        int y = bounds.getY();

        for (const auto* track : tracks)
        {
            if (! track->isVisible())
                continue;

            const auto laneBounds = juce::Rectangle<int> (bounds.getX(), y, bounds.getWidth(), trackHeight);
            const bool isEven = (laneIndex % 2) == 0;
            const auto laneColour = isEven
                ? juce::Colour (Colors::surface1)
                : juce::Colour (Colors::surface2);

            g.setColour (laneColour.withAlpha (0.85f));
            g.fillRect (laneBounds);

            ++laneIndex;
            y += trackHeight;
        }
    }

    // Beat and bar grid lines
    for (int beat = 0; beat < totalBeats; ++beat)
    {
        const auto x = xFromBeats (static_cast<double> (beat));
        const bool isBar = (beat % beatsPerBar) == 0;
        const float alpha = isBar ? 0.35f : 0.15f;

        g.setColour (juce::Colour (Colors::divider).withAlpha (alpha));
        g.drawVerticalLine (x,
                            static_cast<float> (bounds.getY()),
                            static_cast<float> (bounds.getBottom()));
    }
}

void ArrangeView::drawTracks(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    if (projectModel == nullptr)
        return;

    const auto tracks = projectModel->getTracks();
    auto y = bounds.getY();
    int visualIndex = 0;

    for (const auto* track : tracks)
    {
        if (!track->isVisible())
            continue;

        auto trackBounds = juce::Rectangle<int>(bounds.getX(), y, bounds.getWidth(), trackHeight);
        auto trackBoundsFloat = trackBounds.toFloat();

        // Glassmorphism for track header area (left side)
        auto headerBounds = trackBoundsFloat.removeFromLeft(Layout::kTrackHeaderWidth);
        drawGlassPanel(g, headerBounds, Radii::none, false);

        // Track colour accent strip
        const auto accentColour = Tracks::colourForIndex(visualIndex);
        g.setColour(accentColour);
        g.fillRect(headerBounds.withWidth(4.0f));

        // Divider line
        g.setColour(juce::Colour(Colors::divider));
        g.drawHorizontalLine(trackBounds.getBottom(), static_cast<float>(trackBounds.getX()), static_cast<float>(trackBounds.getRight()));

        // Enhanced track name with better typography and shadow
        const auto nameBounds = headerBounds.reduced(Spacing::small);
        const auto font = getBodyFont(Typography::body);
        drawTextWithShadow(g, track->getName(), nameBounds, juce::Justification::centredLeft,
                          font, juce::Colour(Colors::textSoft), 1.0f, 0.25f);

        y += trackHeight;
        ++visualIndex;
    }
}

void ArrangeView::drawContainers(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    if (projectModel == nullptr)
        return;

    const auto containers = projectModel->getContainers();

    for (const auto* container : containers)
    {
        if (container->isCollapsed())
            continue; // Don't draw collapsed containers

        // Get all clips in this container
        const auto& clipIds = container->getClips();
        if (clipIds.empty())
            continue;

        // Calculate bounding box for all clips in container
        juce::Rectangle<int> containerBounds;
        bool first = true;

        for (const auto clipId : clipIds)
        {
            const auto* clip = projectModel->getClip(clipId);
            if (clip == nullptr)
                continue;

            const auto clipBounds = getClipBounds(clip);
            if (!bounds.intersects(clipBounds))
                continue;

            if (first)
            {
                containerBounds = clipBounds;
                first = false;
            }
            else
            {
                containerBounds = containerBounds.getUnion(clipBounds);
            }
        }

        if (containerBounds.isEmpty())
            continue;

        // Draw container background with container color (subtle)
        auto containerColor = container->getColor().withAlpha(0.15f);
        auto containerBoundsFloat = containerBounds.toFloat().expanded(2.0f);
        g.setColour(containerColor);
        g.fillRoundedRectangle(containerBoundsFloat, Radii::small);

        // Draw container border
        g.setColour(container->getColor().withAlpha(0.4f));
        g.drawRoundedRectangle(containerBoundsFloat, Radii::small, 1.5f);
    }
}

void ArrangeView::drawClips(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    if (projectModel == nullptr)
        return;

    const auto& selection = projectModel->getSelectionModel();
    const auto clips = projectModel->getClips();

    for (const auto* clip : clips)
    {
        const auto* track = projectModel->getTrack(clip->getTrackId());
        if (track == nullptr || !track->isVisible())
            continue;

        const auto clipBounds = getClipBounds(clip);
        if (!bounds.intersects(clipBounds))
            continue;

        const auto isSelected = selection.isClipSelected(clip->getId());

        // Check if clip is in a container
        const auto* container = projectModel->getContainerForClip(clip->getId());
        // Base clip colour derived from track colour for consistency
        auto clipColor = Tracks::colourForIndex(clip->getTrackId());
        if (container != nullptr && !container->isCollapsed())
        {
            // Use container color if in a container
            clipColor = container->getColor().interpolatedWith(track->getColor(), 0.7f);
        }

        auto clipBoundsFloat = clipBounds.toFloat();

        // Shadow for depth
        if (isSelected)
        {
            applyShadow(g, Shadows::elevation2, clipBoundsFloat);
        }
        else
        {
            applyShadow(g, Shadows::elevation1, clipBoundsFloat);
        }

        // Clip background with gradient
        if (isSelected)
            clipColor = clipColor.brighter(0.2f);
        juce::ColourGradient clipGradient(clipColor.brighter(0.1f),
                                         clipBoundsFloat.getX(),
                                         clipBoundsFloat.getY(),
                                         clipColor.darker(0.1f),
                                         clipBoundsFloat.getX(),
                                         clipBoundsFloat.getBottom(),
                                         false);
        g.setGradientFill(clipGradient);
        g.fillRoundedRectangle(clipBoundsFloat, Radii::small);

        // Border
        g.setColour(isSelected ? juce::Colour(Colors::outlineFocus) : clipColor.withAlpha(0.5f));
        g.drawRoundedRectangle(clipBoundsFloat, Radii::small, isSelected ? 2.0f : 1.0f);

        // Enhanced clip label with shadow
        const auto labelBounds = clipBounds.reduced(Spacing::xsmall).toFloat();
        const auto font = getBodyFont(Typography::caption);
        drawTextWithShadow(g, clip->getLabel(), labelBounds, juce::Justification::centred,
                          font, juce::Colour(Colors::textSoft), 1.0f, 0.3f);

        // Draw resize handles for selected clips
        if (isSelected)
        {
            drawClipResizeHandles(g, clip, clipBounds, isSelected);
        }

        // Draw fade handles for clips with fade in/out
        drawClipFadeHandles(g, clip, clipBounds);
    }
}

juce::Rectangle<int> ArrangeView::getClipBounds(const daw::project::Clip* clip) const
{
    if (projectModel == nullptr)
        return {};

    const auto* track = projectModel->getTrack(clip->getTrackId());
    if (track == nullptr)
        return {};

    const auto tracks = projectModel->getTracks();
    int trackIndex = 0;
    for (const auto* t : tracks)
    {
        if (t->getId() == track->getId())
            break;
        if (t->isVisible())
            ++trackIndex;
    }

    const auto x = xFromBeats(clip->getStartBeats());
    const auto width = static_cast<int>(clip->getLengthBeats() * pixelsPerBeat);
    const auto y = rulerHeight + trackIndex * trackHeight;

    return juce::Rectangle<int>(x, y, width, trackHeight);
}

daw::project::Clip* ArrangeView::getClipAtPosition(juce::Point<int> pos) const
{
    if (projectModel == nullptr)
        return nullptr;

    if (pos.y < rulerHeight)
        return nullptr;

    const auto clips = projectModel->getClips();
    for (auto* clip : clips)
    {
        const auto clipBounds = getClipBounds(clip);
        if (clipBounds.contains(pos))
            return clip;
    }

    return nullptr;
}

bool ArrangeView::isNearClipEdge(juce::Point<int> pos, const daw::project::Clip* clip, bool& isStartEdge) const
{
    if (clip == nullptr)
        return false;

    const auto clipBounds = getClipBounds(clip);
    constexpr int edgeThreshold = 8; // pixels

    // Check if near left edge (start)
    if (std::abs(pos.x - clipBounds.getX()) < edgeThreshold &&
        pos.y >= clipBounds.getY() && pos.y < clipBounds.getBottom())
    {
        isStartEdge = true;
        return true;
    }

    // Check if near right edge (end)
    if (std::abs(pos.x - clipBounds.getRight()) < edgeThreshold &&
        pos.y >= clipBounds.getY() && pos.y < clipBounds.getBottom())
    {
        isStartEdge = false;
        return true;
    }

    return false;
}

double ArrangeView::beatsFromX(int x) const
{
    return static_cast<double>(x) / pixelsPerBeat;
}

int ArrangeView::xFromBeats(double beats) const
{
    return static_cast<int>(beats * pixelsPerBeat);
}

double ArrangeView::snapBeats(double beats) const
{
    if (!snapEnabled)
        return beats;

    return std::round(beats / snapDivision) * snapDivision;
}

std::vector<daw::project::Clip*> ArrangeView::getClipsInRect(juce::Rectangle<int> rect) const
{
    std::vector<daw::project::Clip*> result;

    if (projectModel == nullptr)
        return result;

    const auto clips = projectModel->getClips();
    for (auto* clip : clips)
    {
        const auto clipBounds = getClipBounds(clip);
        if (rect.intersects(clipBounds))
        {
            result.push_back(clip);
        }
    }

    return result;
}

void ArrangeView::drawBoxSelection(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (!isBoxSelecting || boxSelectRect.isEmpty())
        return;

    using namespace daw::ui::lookandfeel::DesignSystem;

    auto selectionRect = boxSelectRect.toFloat();
    if (!bounds.toFloat().intersects(selectionRect))
        return;

    // Draw selection rectangle with glassmorphism
    g.setColour(juce::Colour(Colors::accent).withAlpha(0.2f));
    g.fillRect(selectionRect);

    // Draw border
    g.setColour(juce::Colour(Colors::accent));
    g.drawRect(selectionRect, 2.0f);

    // Draw dashed border for visual feedback
    juce::Path dashedPath;
    dashedPath.addRectangle(selectionRect);
    juce::PathStrokeType stroke(1.5f);
    float dashes[] = { 4.0f, 4.0f };
    stroke.createDashedStroke(dashedPath, dashedPath, dashes, 2);
    g.setColour(juce::Colour(Colors::accent).withAlpha(0.6f));
    g.strokePath(dashedPath, stroke);
}

bool ArrangeView::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    const auto description = dragSourceDetails.description.toString();
    return description.startsWith("BrowserItem:");
}

void ArrangeView::itemDropped(const SourceDetails& dragSourceDetails)
{
    if (projectModel == nullptr)
        return;

    const auto description = dragSourceDetails.description.toString();
    if (!description.startsWith("BrowserItem:"))
        return;

    // Parse: "BrowserItem:name:tabIndex"
    juce::StringArray parts;
    parts.addTokens(description, ":", "");
    if (parts.size() < 3)
        return;

    const auto itemName = parts[1];
    const auto tabIndex = parts[2].getIntValue();

    // Get drop position
    const auto dropPos = getMouseXYRelative();
    if (dropPos.y < rulerHeight)
        return; // Dropped in ruler, ignore

    // Find track at drop position
    const auto tracks = projectModel->getTracks();
    const auto trackIndex = (dropPos.y - rulerHeight) / trackHeight;
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size()))
        return;

    const auto* track = tracks[static_cast<size_t>(trackIndex)];
    if (track == nullptr)
        return;

    // Calculate start position in beats
    const auto startBeats = beatsFromX(dropPos.x);
    const auto snappedStart = snapBeats(startBeats);

    // Handle different tab types
    if (tabIndex == 3) // SamplesTab
    {
        // Create audio clip from sample
        const auto lengthBeats = 4.0; // Default 4 beats
        projectModel->addClip(track->getId(), snappedStart, lengthBeats, itemName.toStdString());
    }
    else if (tabIndex == 0) // Current project (tracks)
    {
        // Create pattern clip
        const auto lengthBeats = 4.0;
        auto* clip = projectModel->addClip(track->getId(), snappedStart, lengthBeats, itemName.toStdString());

        // Create pattern and link to clip
        auto* pattern = projectModel->addPattern(itemName.toStdString() + " Pattern");
        if (clip != nullptr && pattern != nullptr)
        {
            projectModel->linkClipToPattern(clip->getId(), pattern->getId());
        }
    }

    refresh();
}

void ArrangeView::updateTrackHeaders()
{
    // Clear existing headers
    trackHeaders.clear();

    if (!projectModel)
        return;

    const auto tracks = projectModel->getTracks();

    // Create header for each track
    int trackIndex = 0;
    for (auto* track : tracks)
    {
        if (!track->isVisible())
            continue;

        // Set track index for proper callbacks
        track->setIndex(trackIndex);

        auto trackHeader = std::make_unique<TrackHeaderComponent>(engineContext, track);

        // Set up callbacks
        trackHeader->onMuteChanged = [this](int trackIndex, bool muted)
        {
            // Update project model
            if (projectModel)
            {
                auto* track = projectModel->getTrackByIndex(trackIndex);
                if (track)
                    track->setMuted(muted);
            }
        };

        trackHeader->onSoloChanged = [this](int trackIndex, bool soloed)
        {
            // Update project model
            if (projectModel)
            {
                auto* track = projectModel->getTrackByIndex(trackIndex);
                if (track)
                    track->setSoloed(soloed);
            }
        };

        trackHeader->onRecordArmChanged = [this](int trackIndex, bool armed)
        {
            // Update project model
            if (projectModel)
            {
                auto* track = projectModel->getTrackByIndex(trackIndex);
                if (track)
                    track->setRecordArmed(armed);
            }
        };

        trackHeader->onVolumeChanged = [this](int trackIndex, float gainDb)
        {
            // Update project model
            if (projectModel)
            {
                auto* track = projectModel->getTrackByIndex(trackIndex);
                if (track)
                    track->setGainDb(gainDb);
            }
        };

        trackHeader->onPanChanged = [this](int trackIndex, float pan)
        {
            // Update project model
            if (projectModel)
            {
                auto* track = projectModel->getTrackByIndex(trackIndex);
                if (track)
                    track->setPan(pan);
            }
        };

        trackHeader->onNameChanged = [this](int trackIndex, const juce::String& newName)
        {
            // Update project model
            if (projectModel)
            {
                auto* track = projectModel->getTrackByIndex(trackIndex);
                if (track)
                    track->setName(newName.toStdString());
            }
        };        addAndMakeVisible(*trackHeader);
        trackHeaders.push_back(std::move(trackHeader));

        ++trackIndex;
    }

    resized(); // Update layout
}

void ArrangeView::refreshTrackHeaders()
{
    for (auto& header : trackHeaders)
    {
        header->refresh();
    }
}

void ArrangeView::drawClipResizeHandles(juce::Graphics& g, const daw::project::Clip* clip, juce::Rectangle<int> clipBounds, bool isSelected)
{
    if (!isSelected)
        return;
        
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Resize handles at left and right edges
    const auto handleHeight = clipBounds.getHeight();
    const auto leftHandle = juce::Rectangle<int>(clipBounds.getX(), clipBounds.getY(), kResizeHandleWidth, handleHeight);
    const auto rightHandle = juce::Rectangle<int>(clipBounds.getRight() - kResizeHandleWidth, clipBounds.getY(), kResizeHandleWidth, handleHeight);
    
    // Handle styling
    const auto handleColour = juce::Colour(Colors::outlineFocus).withAlpha(0.8f);
    
    // Draw left resize handle
    g.setColour(handleColour);
    g.fillRoundedRectangle(leftHandle.toFloat(), 2.0f);
    
    // Draw resize indicator lines
    g.setColour(juce::Colour(Colors::background));
    for (int i = 1; i < 4; ++i)
    {
        const int lineX = leftHandle.getX() + (i * 2);
        g.drawVerticalLine(lineX, static_cast<float>(leftHandle.getY() + 2), static_cast<float>(leftHandle.getBottom() - 2));
    }
    
    // Draw right resize handle  
    g.setColour(handleColour);
    g.fillRoundedRectangle(rightHandle.toFloat(), 2.0f);
    
    // Draw resize indicator lines
    g.setColour(juce::Colour(Colors::background));
    for (int i = 1; i < 4; ++i)
    {
        const int lineX = rightHandle.getX() + (i * 2);
        g.drawVerticalLine(lineX, static_cast<float>(rightHandle.getY() + 2), static_cast<float>(rightHandle.getBottom() - 2));
    }
}

void ArrangeView::drawClipFadeHandles(juce::Graphics& g, const daw::project::Clip* clip, juce::Rectangle<int> clipBounds)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Check if clip has fade in/out (for now, assume all clips can have fades)
    const auto fadeInLength = 0.25; // TODO: Get from clip model
    const auto fadeOutLength = 0.25; // TODO: Get from clip model
    
    if (fadeInLength > 0.0)
    {
        // Draw fade in visualization
        const int fadeInPixels = static_cast<int>(fadeInLength * pixelsPerBeat);
        const auto fadeInBounds = clipBounds.withWidth(juce::jmin(fadeInPixels, clipBounds.getWidth() / 3));
        
        // Fade in gradient overlay
        juce::ColourGradient fadeInGradient(juce::Colour(Colors::surface1).withAlpha(0.6f),
                                           static_cast<float>(fadeInBounds.getX()),
                                           static_cast<float>(fadeInBounds.getCentreY()),
                                           juce::Colours::transparentWhite,
                                           static_cast<float>(fadeInBounds.getRight()),
                                           static_cast<float>(fadeInBounds.getCentreY()),
                                           false);
        g.setGradientFill(fadeInGradient);
        g.fillRect(fadeInBounds);
        
        // Fade in curve line
        juce::Path fadeInCurve;
        fadeInCurve.startNewSubPath(static_cast<float>(fadeInBounds.getX()), static_cast<float>(fadeInBounds.getBottom()));
        fadeInCurve.quadraticTo(static_cast<float>(fadeInBounds.getCentreX()), static_cast<float>(fadeInBounds.getY()),
                               static_cast<float>(fadeInBounds.getRight()), static_cast<float>(fadeInBounds.getY()));
        
        g.setColour(juce::Colour(Colors::outline).withAlpha(0.7f));
        g.strokePath(fadeInCurve, juce::PathStrokeType(1.5f));
    }
    
    if (fadeOutLength > 0.0)
    {
        // Draw fade out visualization
        const int fadeOutPixels = static_cast<int>(fadeOutLength * pixelsPerBeat);
        const auto fadeOutBounds = clipBounds.withX(clipBounds.getRight() - juce::jmin(fadeOutPixels, clipBounds.getWidth() / 3))
                                            .withWidth(juce::jmin(fadeOutPixels, clipBounds.getWidth() / 3));
        
        // Fade out gradient overlay
        juce::ColourGradient fadeOutGradient(juce::Colours::transparentWhite,
                                            static_cast<float>(fadeOutBounds.getX()),
                                            static_cast<float>(fadeOutBounds.getCentreY()),
                                            juce::Colour(Colors::surface1).withAlpha(0.6f),
                                            static_cast<float>(fadeOutBounds.getRight()),
                                            static_cast<float>(fadeOutBounds.getCentreY()),
                                            false);
        g.setGradientFill(fadeOutGradient);
        g.fillRect(fadeOutBounds);
        
        // Fade out curve line
        juce::Path fadeOutCurve;
        fadeOutCurve.startNewSubPath(static_cast<float>(fadeOutBounds.getX()), static_cast<float>(fadeOutBounds.getY()));
        fadeOutCurve.quadraticTo(static_cast<float>(fadeOutBounds.getCentreX()), static_cast<float>(fadeOutBounds.getBottom()),
                                static_cast<float>(fadeOutBounds.getRight()), static_cast<float>(fadeOutBounds.getBottom()));
        
        g.setColour(juce::Colour(Colors::outline).withAlpha(0.7f));
        g.strokePath(fadeOutCurve, juce::PathStrokeType(1.5f));
    }
}

bool ArrangeView::isOverResizeHandle(juce::Point<int> pos, const daw::project::Clip* clip, bool& isStartHandle)
{
    const auto clipBounds = getClipBounds(clip);
    const auto leftHandle = juce::Rectangle<int>(clipBounds.getX(), clipBounds.getY(), kResizeHandleWidth, clipBounds.getHeight());
    const auto rightHandle = juce::Rectangle<int>(clipBounds.getRight() - kResizeHandleWidth, clipBounds.getY(), kResizeHandleWidth, clipBounds.getHeight());
    
    if (leftHandle.contains(pos))
    {
        isStartHandle = true;
        return true;
    }
    else if (rightHandle.contains(pos))
    {
        isStartHandle = false;
        return true;
    }
    
    return false;
}

bool ArrangeView::isOverFadeHandle(juce::Point<int> pos, const daw::project::Clip* clip, bool& isStartHandle)
{
    const auto clipBounds = getClipBounds(clip);
    const auto fadeInBounds = clipBounds.withWidth(kFadeHandleWidth);
    const auto fadeOutBounds = clipBounds.withX(clipBounds.getRight() - kFadeHandleWidth).withWidth(kFadeHandleWidth);
    
    if (fadeInBounds.contains(pos))
    {
        isStartHandle = true;
        return true;
    }
    else if (fadeOutBounds.contains(pos))
    {
        isStartHandle = false;  
        return true;
    }
    
    return false;
}

juce::Colour ArrangeView::getClipTypeColour(const daw::project::Clip* clip)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Use clip's color index if set
    if (clip->getColorIndex() > 0)
    {
        const auto colors = std::vector<juce::Colour>{
            juce::Colour(Colors::primary),
            juce::Colour(Colors::secondary), 
            juce::Colour(Colors::warning),
            juce::Colour(Colors::danger),
            juce::Colour(Colors::success),
            juce::Colour(Colors::meterNormal)
        };
        
        const int index = clip->getColorIndex();
        if (index > 0 && index <= static_cast<int>(colors.size()))
        {
            return colors[index - 1];
        }
    }
    
    // Fallback to track-based coloring
    if (projectModel)
    {
        const auto* track = projectModel->getTrack(clip->getTrackId());
        if (track)
            return Tracks::colourForIndex(clip->getTrackId());
    }
    
    // Default color
    return juce::Colour(Colors::primary);
}

void ArrangeView::showClipContextMenu(const daw::project::Clip* clip, juce::Point<int> position)
{
    juce::PopupMenu contextMenu;
    
    // Basic clip operations
    contextMenu.addItem(1, "Cut", true);
    contextMenu.addItem(2, "Copy", true);
    contextMenu.addItem(3, "Paste", true);
    contextMenu.addSeparator();
    
    contextMenu.addItem(4, "Delete", true);
    contextMenu.addItem(5, "Duplicate", true);
    contextMenu.addSeparator();
    
    // Clip-specific operations
    contextMenu.addItem(6, "Split at Playhead", true);
    contextMenu.addItem(7, "Reverse", true);
    contextMenu.addItem(8, "Normalize", true);
    contextMenu.addSeparator();
    
    // Fade operations
    contextMenu.addItem(9, "Fade In...", true);
    contextMenu.addItem(10, "Fade Out...", true);
    contextMenu.addSeparator();
    
    // Color menu
    juce::PopupMenu colorMenu;
    const auto colors = std::vector<juce::Colour>{
        juce::Colour(Colors::primary),
        juce::Colour(Colors::secondary), 
        juce::Colour(Colors::warning),
        juce::Colour(Colors::danger),
        juce::Colour(Colors::success),
        juce::Colour(Colors::meterNormal)
    };
    
    for (size_t i = 0; i < colors.size(); ++i)
    {
        colorMenu.addColouredItem(static_cast<int>(20 + i), "Color " + juce::String(i + 1), colors[i], true);
    }
    
    contextMenu.addSubMenu("Set Color", colorMenu);
    
    // Show menu
    const auto result = contextMenu.show();
    
    if (result > 0)
    {
        handleClipContextMenuResult(result, clip);
    }
}

void ArrangeView::handleClipContextMenuResult(int result, const daw::project::Clip* clip)
{
    if (!projectModel || !clip)
        return;
        
    switch (result)
    {
        case 1: // Cut
            cutSelectedClips();
            break;
            
        case 2: // Copy  
            copySelectedClips();
            break;
            
        case 3: // Paste
            pasteClips();
            break;
            
        case 4: // Delete
            projectModel->removeClip(clip->getId());
            refresh();
            break;
            
        case 5: // Duplicate
            duplicateSelectedClips();
            break;
            
        case 6: // Split at Playhead
            splitClipAtPlayhead(clip);
            break;
            
        case 7: // Reverse
            // TODO: Implement reverse operation
            break;
            
        case 8: // Normalize
            // TODO: Implement normalize operation
            break;
            
        case 9: // Fade In
            // TODO: Implement fade in dialog
            break;
            
        case 10: // Fade Out
            // TODO: Implement fade out dialog
            break;
            
        default:
            // Handle color selection (20+)
            if (result >= 20 && result < 30)
            {
                const int colorIndex = result - 20;
                setClipColor(clip, colorIndex);
                refresh();
            }
            break;
    }
}

void ArrangeView::cutSelectedClips()
{
    if (!projectModel) return;
    
    const auto& selection = projectModel->getSelectionModel();
    const auto selectedClips = selection.getSelectedClips();
    
    if (selectedClips.empty()) return;
    
    // Clear previous clipboard
    clipboard.clips.clear();
    clipboard.isCutOperation = true;
    
    // Find the earliest clip position for relative pasting
    double earliestStart = std::numeric_limits<double>::max();
    for (const auto clipId : selectedClips)
    {
        if (auto* clip = projectModel->getClip(clipId))
        {
            earliestStart = std::min(earliestStart, clip->getStartBeats());
        }
    }
    clipboard.originBeats = earliestStart;
    
    // Copy clips to clipboard and remove from project
    for (const auto clipId : selectedClips)
    {
        if (auto* clip = projectModel->getClip(clipId))
        {
            clipboard.clips.push_back(*clip);
            projectModel->removeClip(clipId);
        }
    }
    
    refresh();
}

void ArrangeView::copySelectedClips()
{
    if (!projectModel) return;
    
    const auto& selection = projectModel->getSelectionModel();
    const auto selectedClips = selection.getSelectedClips();
    
    if (selectedClips.empty()) return;
    
    // Clear previous clipboard
    clipboard.clips.clear();
    clipboard.isCutOperation = false;
    
    // Find the earliest clip position for relative pasting
    double earliestStart = std::numeric_limits<double>::max();
    for (const auto clipId : selectedClips)
    {
        if (auto* clip = projectModel->getClip(clipId))
        {
            earliestStart = std::min(earliestStart, clip->getStartBeats());
        }
    }
    clipboard.originBeats = earliestStart;
    
    // Copy clips to clipboard
    for (const auto clipId : selectedClips)
    {
        if (auto* clip = projectModel->getClip(clipId))
        {
            clipboard.clips.push_back(*clip);
        }
    }
}

void ArrangeView::pasteClips()
{
    if (!projectModel || clipboard.clips.empty()) return;
    
    // Get current playhead position or mouse position
    // For now, use a simple offset from the original position
    double pastePosition = clipboard.originBeats + 4.0; // Paste 4 beats after original
    
    auto& selection = projectModel->getSelectionModel();
    selection.clearAll();
    
    // Paste clips with offset
    for (const auto& clipData : clipboard.clips)
    {
        auto newClip = clipData;
        double offset = pastePosition - clipboard.originBeats;
        newClip.setStartBeats(clipData.getStartBeats() + offset);
        
        auto* addedClip = projectModel->addClip(std::move(newClip));
        if (addedClip)
        {
            selection.selectClip(addedClip->getId());
        }
    }
    
    refresh();
}

void ArrangeView::duplicateSelectedClips()
{
    if (!projectModel) return;
    
    const auto& selection = projectModel->getSelectionModel();
    const auto selectedClips = selection.getSelectedClips();
    
    if (selectedClips.empty()) return;
    
    // Calculate offset for duplicated clips
    double maxEnd = 0.0;
    for (const auto clipId : selectedClips)
    {
        if (auto* clip = projectModel->getClip(clipId))
        {
            maxEnd = std::max(maxEnd, clip->getEndBeats());
        }
    }
    
    auto& newSelection = const_cast<daw::project::SelectionModel&>(selection);
    newSelection.clearAll();
    
    // Duplicate clips
    for (const auto clipId : selectedClips)
    {
        if (auto* clip = projectModel->getClip(clipId))
        {
            auto duplicatedClip = *clip;
            duplicatedClip.setStartBeats(maxEnd + 0.1); // Small gap after original clips
            
            auto* addedClip = projectModel->addClip(std::move(duplicatedClip));
            if (addedClip)
            {
                newSelection.selectClip(addedClip->getId());
            }
        }
    }
    
    refresh();
}

void ArrangeView::splitClipAtPlayhead(const daw::project::Clip* clip)
{
    if (!projectModel || !clip || !engineContext) return;
    
    // Get current playhead position
    const auto currentPosition = engineContext->getPlayheadBeats();
    
    // Check if playhead is within the clip
    if (currentPosition <= clip->getStartBeats() || currentPosition >= clip->getEndBeats())
        return; // Playhead not within clip
    
    // Create two new clips from the split
    auto leftClip = *clip;
    auto rightClip = *clip;
    
    // Left clip: from start to playhead
    leftClip.setLengthBeats(currentPosition - clip->getStartBeats());
    
    // Right clip: from playhead to end
    rightClip.setStartBeats(currentPosition);
    rightClip.setLengthBeats(clip->getEndBeats() - currentPosition);
    
    // Remove original clip and add split clips
    projectModel->removeClip(clip->getId());
    projectModel->addClip(std::move(leftClip));
    projectModel->addClip(std::move(rightClip));
    
    refresh();
}

void ArrangeView::deleteSelectedClips()
{
    if (!projectModel || undoManager == nullptr) return;

    std::vector<uint32_t> selectedClipIds;
    selectedClipIds.reserve(16);

    for (const auto& track : projectModel->getTracks())
    {
        for (const auto& clip : track->getClips())
        {
            if (clip->isSelected())
            {
                selectedClipIds.push_back(clip->getId());
            }
        }
    }

    if (selectedClipIds.empty())
        return;

    // Delete clips in reverse order to keep deterministic command ordering per track
    std::sort(selectedClipIds.rbegin(), selectedClipIds.rend());

    for (uint32_t clipId : selectedClipIds)
    {
        auto command = std::make_unique<daw::project::RemoveClipCommand>(clipId);
        undoManager->executeCommand(std::move(command), *projectModel);
    }

    refresh();
}

void ArrangeView::setClipColor(const daw::project::Clip* clip, int colorIndex)
{
    if (!projectModel || !clip) return;
    
    // Find the clip and update its color
    if (auto* mutableClip = projectModel->getClip(clip->getId()))
    {
        mutableClip->setColorIndex(colorIndex);
        refresh();
    }
}

} // namespace daw::ui::views
