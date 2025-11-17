#include "PianoRollView.h"
#include "../../project/Track.h"
#include "../../project/Clip.h"
#include <algorithm>
#include <cmath>

namespace daw::ui::views
{

PianoRollView::PianoRollView()
{
    setInterceptsMouseClicks(true, true);
    startTimer(30); // 30fps update rate
}

void PianoRollView::paint(juce::Graphics& g)
{
    using namespace daw::ui::lookandfeel::DesignSystem;
    
    // Background
    g.fillAll(toColour(Colors::background));
    
    const auto bounds = getLocalBounds().toFloat();
    
    // Draw grid
    g.setColour(toColour(Colors::outline));
    const auto numBeats = static_cast<int>(std::ceil(viewEndBeat - viewStartBeat));
    const auto numNotes = viewEndNote - viewStartNote;
    
    // Vertical lines (beats)
    for (int i = 0; i <= numBeats; ++i)
    {
        const auto beat = viewStartBeat + i;
        const auto x = (beat - viewStartBeat) * pixelsPerBeat;
        
        if (x >= 0 && x <= bounds.getWidth())
        {
            const bool isStrongBeat = (static_cast<int>(beat) % 4) == 0;
            g.setColour(toColour(isStrongBeat ? Colors::outline : Colors::divider));
            g.drawVerticalLine(static_cast<int>(x), 0.0f, bounds.getHeight());
        }
    }
    
    // Horizontal lines (notes)
    for (int i = 0; i <= numNotes; ++i)
    {
        const auto note = viewStartNote + i;
        const auto y = (viewEndNote - note) * pixelsPerNote;
        
        if (y >= 0 && y <= bounds.getHeight())
        {
            const bool isOctave = (note % 12) == 0;
            g.setColour(toColour(isOctave ? Colors::outline : Colors::divider));
            g.drawHorizontalLine(static_cast<int>(y), 0.0f, bounds.getWidth());
        }
    }
    
    // Draw ghost notes
    if (showGhostNotes)
    {
        g.setColour(toColour(Colors::textSecondary).withAlpha(0.3f));
        for (const auto& note : ghostNotes)
        {
            g.fillRoundedRectangle(note.bounds, Radii::small);
        }
    }
    
    // Draw notes
    for (const auto& note : notes)
    {
        if (note.isSelected)
        {
            g.setColour(toColour(Colors::primary));
        }
        else
        {
            g.setColour(toColour(Colors::accent));
        }
        g.fillRoundedRectangle(note.bounds, Radii::small);
        
        // Note border
        g.setColour(toColour(Colors::outline));
        g.drawRoundedRectangle(note.bounds, Radii::small, 1.0f);
    }
    
    // Draw hover indicator
    if (hoveredNote >= 0 && hoveredNote < static_cast<int>(notes.size()))
    {
        g.setColour(toColour(Colors::primary).withAlpha(0.3f));
        g.fillRoundedRectangle(notes[hoveredNote].bounds, Radii::small);
    }
}

void PianoRollView::resized()
{
    repaint();
}

void PianoRollView::mouseDown(const juce::MouseEvent& e)
{
    const auto [note, beat] = rectToNote(e.getPosition());
    
    // Find note at this position
    auto it = std::find_if(notes.begin(), notes.end(),
        [note, beat](const NoteRect& n)
        {
            return n.note == note && n.startBeat <= beat && (n.startBeat + n.lengthBeats) > beat;
        });
    
    if (it != notes.end())
    {
        // Start dragging existing note
        isDragging = true;
        dragStart = e.getPosition();
    }
    else
    {
        // Create new note
        NoteRect newNote;
        newNote.note = snapNoteToScale(note);
        newNote.startBeat = snapBeatToGrid(beat);
        newNote.lengthBeats = quantization;
        newNote.bounds = noteToRect(newNote.note, newNote.startBeat, newNote.lengthBeats);
        newNote.isSelected = true;
        
        notes.push_back(newNote);
        isDragging = true;
        dragStart = e.getPosition();
        
        // TODO: Add note to project model
    }
    
    repaint();
}

void PianoRollView::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging || notes.empty())
        return;
    
    auto& note = notes.back();
    const auto [newNote, newBeat] = rectToNote(e.getPosition());
    
    note.note = snapNoteToScale(newNote);
    note.startBeat = snapBeatToGrid(newBeat);
    note.bounds = noteToRect(note.note, note.startBeat, note.lengthBeats);
    
    repaint();
}

void PianoRollView::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    isDragging = false;
    repaint();
}

void PianoRollView::mouseMove(const juce::MouseEvent& e)
{
    const auto [note, beat] = rectToNote(e.getPosition());
    
    // Find hovered note
    hoveredNote = -1;
    for (size_t i = 0; i < notes.size(); ++i)
    {
        const auto& n = notes[i];
        if (n.note == note && n.startBeat <= beat && (n.startBeat + n.lengthBeats) > beat)
        {
            hoveredNote = static_cast<int>(i);
            break;
        }
    }
    
    repaint();
}

void PianoRollView::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        // Zoom
        const auto zoomFactor = wheel.deltaY > 0 ? 1.1f : 0.9f;
        pixelsPerBeat *= zoomFactor;
        pixelsPerBeat = juce::jlimit(10.0f, 200.0f, pixelsPerBeat);
    }
    else
    {
        // Scroll
        const auto scrollAmount = wheel.deltaY * 20.0f;
        viewStartBeat += scrollAmount / pixelsPerBeat;
        viewStartBeat = juce::jmax(0.0, viewStartBeat);
    }
    
    repaint();
}

void PianoRollView::setProjectModel(std::shared_ptr<daw::project::ProjectModel> model)
{
    projectModel = model;
    updateNotes();
}

void PianoRollView::setCurrentTrack(uint32_t trackId)
{
    currentTrackId = trackId;
    updateNotes();
}

void PianoRollView::setQuantization(double gridDivision)
{
    quantization = gridDivision;
    repaint();
}

void PianoRollView::setScaleSnapping(bool enabled, const std::vector<int>& scaleIntervals)
{
    scaleSnapping = enabled;
    this->scaleIntervals = scaleIntervals;
    repaint();
}

void PianoRollView::setShowGhostNotes(bool show, uint32_t referenceTrackId)
{
    showGhostNotes = show;
    ghostTrackId = referenceTrackId;
    updateGhostNotes();
}

void PianoRollView::timerCallback()
{
    // Update notes from project model
    updateNotes();
}

void PianoRollView::updateNotes()
{
    notes.clear();
    
    if (!projectModel)
        return;
    
    const auto* track = projectModel->getTrack(currentTrackId);
    if (!track)
        return;
    
    // TODO: Get MIDI notes from track/clips and convert to NoteRect
    // For now, this is a placeholder
}

void PianoRollView::updateGhostNotes()
{
    ghostNotes.clear();
    
    if (!showGhostNotes || !projectModel)
        return;
    
    const auto* track = projectModel->getTrack(ghostTrackId);
    if (!track)
        return;
    
    // TODO: Get MIDI notes from ghost track and convert to NoteRect
    // For now, this is a placeholder
}

juce::Rectangle<float> PianoRollView::noteToRect(int note, double startBeat, double lengthBeats) const
{
    const auto x = static_cast<float>((startBeat - viewStartBeat) * pixelsPerBeat);
    const auto y = static_cast<float>((viewEndNote - note) * pixelsPerNote);
    const auto width = static_cast<float>(lengthBeats * pixelsPerBeat);
    const auto height = pixelsPerNote;
    
    return juce::Rectangle<float>(x, y, width, height);
}

std::pair<int, double> PianoRollView::rectToNote(const juce::Point<int>& point) const
{
    const auto note = viewEndNote - static_cast<int>(point.y / pixelsPerNote);
    const auto beat = viewStartBeat + (point.x / pixelsPerBeat);
    
    return {note, beat};
}

int PianoRollView::snapNoteToScale(int note) const
{
    if (!scaleSnapping || scaleIntervals.empty())
        return note;
    
    const auto rootNote = note % 12;
    const auto octave = note / 12;
    
    // Find closest scale interval
    int closestInterval = 0;
    int minDistance = 12;
    
    for (int interval : scaleIntervals)
    {
        const auto distance = std::abs(rootNote - interval);
        if (distance < minDistance)
        {
            minDistance = distance;
            closestInterval = interval;
        }
    }
    
    return octave * 12 + closestInterval;
}

double PianoRollView::snapBeatToGrid(double beat) const
{
    return std::round(beat / quantization) * quantization;
}

void PianoRollView::repaintNotes()
{
    for (auto& note : notes)
    {
        note.bounds = noteToRect(note.note, note.startBeat, note.lengthBeats);
    }
    repaint();
}

} // namespace daw::ui::views

