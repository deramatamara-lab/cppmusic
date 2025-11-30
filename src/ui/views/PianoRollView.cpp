#include "PianoRollView.h"
#include "../../project/Track.h"
#include "../../project/Clip.h"
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>

namespace daw::ui::views
{

PianoRollView::PianoRollView()
    : aiGenerateMelodyButton("AI Melody")
    , aiGenerateChordsButton("AI Chords")
    , aiGenerateGrooveButton("AI Groove")
{
    setInterceptsMouseClicks(true, true);
    startTimer(30); // 30fps update rate
    setupAIButtons();
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
    using namespace daw::ui::lookandfeel::DesignSystem;

    auto bounds = getLocalBounds();

    // AI buttons at top
    auto aiButtonArea = bounds.removeFromTop(30);
    const int buttonWidth = 80;
    aiGenerateMelodyButton.setBounds(aiButtonArea.removeFromLeft(buttonWidth));
    aiButtonArea.removeFromLeft(Spacing::xsmall);
    aiGenerateChordsButton.setBounds(aiButtonArea.removeFromLeft(buttonWidth));
    aiButtonArea.removeFromLeft(Spacing::xsmall);
    aiGenerateGrooveButton.setBounds(aiButtonArea.removeFromLeft(buttonWidth));

    bounds.removeFromTop(Spacing::xsmall);

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
        isCreatingNote = false;
        draggedNoteIndex = static_cast<int>(std::distance(notes.begin(), it));
        dragStart = e.getPosition();

        // Store original note data for pattern update
        if (projectModel && currentPatternId != 0)
        {
            const auto* pattern = projectModel->getPattern(currentPatternId);
            if (pattern != nullptr)
            {
                const auto& patternNotes = pattern->getNotes();
                if (draggedNoteIndex < static_cast<int>(patternNotes.size()))
                {
                    originalNote = patternNotes[static_cast<size_t>(draggedNoteIndex)];
                }
            }
        }
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
        isCreatingNote = true;
        draggedNoteIndex = static_cast<int>(notes.size() - 1);
        dragStart = e.getPosition();

        // Add note to pattern model
        if (projectModel && currentPatternId != 0)
        {
            auto* pattern = projectModel->getPattern(currentPatternId);
            if (pattern != nullptr)
            {
                daw::project::Pattern::MIDINote midiNote;
                midiNote.note = static_cast<uint8_t>(newNote.note);
                midiNote.velocity = 100;
                midiNote.startBeat = newNote.startBeat;
                midiNote.lengthBeats = newNote.lengthBeats;
                midiNote.channel = 0;
                pattern->addNote(midiNote);

                originalNote = midiNote;
            }
        }
    }

    repaint();
}

void PianoRollView::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging || notes.empty() || draggedNoteIndex < 0 ||
        draggedNoteIndex >= static_cast<int>(notes.size()))
        return;

    auto& note = notes[static_cast<size_t>(draggedNoteIndex)];
    const auto [newNote, newBeat] = rectToNote(e.getPosition());

    note.note = snapNoteToScale(newNote);
    note.startBeat = snapBeatToGrid(newBeat);
    note.bounds = noteToRect(note.note, note.startBeat, note.lengthBeats);

    repaint();
}

void PianoRollView::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);

    if (isDragging && !isCreatingNote && draggedNoteIndex >= 0 &&
        draggedNoteIndex < static_cast<int>(notes.size()) &&
        projectModel && currentPatternId != 0)
    {
        // Update pattern note after drag
        auto* pattern = projectModel->getPattern(currentPatternId);
        if (pattern != nullptr)
        {
            // Remove old note and add updated one
            const auto& patternNotes = pattern->getNotes();
            for (size_t i = 0; i < patternNotes.size(); ++i)
            {
                const auto& pNote = patternNotes[i];
                if (pNote.note == originalNote.note &&
                    std::abs(pNote.startBeat - originalNote.startBeat) < 0.001)
                {
                    pattern->removeNote(i);

                    // Add updated note
                    const auto& note = notes[static_cast<size_t>(draggedNoteIndex)];
                    daw::project::Pattern::MIDINote updatedNote;
                    updatedNote.note = static_cast<uint8_t>(note.note);
                    updatedNote.velocity = originalNote.velocity;
                    updatedNote.startBeat = note.startBeat;
                    updatedNote.lengthBeats = note.lengthBeats;
                    updatedNote.channel = originalNote.channel;
                    pattern->addNote(updatedNote);
                    break;
                }
            }
        }
    }

    isDragging = false;
    isCreatingNote = false;
    draggedNoteIndex = -1;
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
    currentClipId = 0;
    currentPatternId = 0;
    updateNotes();
}

void PianoRollView::setCurrentClip(uint32_t clipId)
{
    currentClipId = clipId;

    if (projectModel)
    {
        const auto* clip = projectModel->getClip(clipId);
        if (clip != nullptr)
        {
            currentTrackId = clip->getTrackId();
            if (clip->hasPattern())
            {
                currentPatternId = clip->getPatternId();
            }
            else
            {
                // Create a new pattern for this clip
                const auto* track = projectModel->getTrack(currentTrackId);
                if (track != nullptr)
                {
                    auto* pattern = projectModel->addPattern(track->getName() + " Pattern");
                    projectModel->linkClipToPattern(clipId, pattern->getId());
                    currentPatternId = pattern->getId();
                }
            }
        }
    }

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

    // Get pattern from current clip
    if (currentClipId != 0)
    {
        const auto* clip = projectModel->getClip(currentClipId);
        if (clip != nullptr && clip->hasPattern())
        {
            currentPatternId = clip->getPatternId();
            const auto* pattern = projectModel->getPattern(currentPatternId);
            if (pattern != nullptr)
            {
                // Convert Pattern MIDI notes to NoteRect
                const auto& patternNotes = pattern->getNotes();
                for (const auto& midiNote : patternNotes)
                {
                    NoteRect noteRect;
                    noteRect.note = midiNote.note;
                    noteRect.startBeat = midiNote.startBeat;
                    noteRect.lengthBeats = midiNote.lengthBeats;
                    noteRect.bounds = noteToRect(noteRect.note, noteRect.startBeat, noteRect.lengthBeats);
                    noteRect.isSelected = false;
                    notes.push_back(noteRect);
                }
            }
        }
    }
    else if (currentPatternId != 0)
    {
        // Direct pattern editing
        const auto* pattern = projectModel->getPattern(currentPatternId);
        if (pattern != nullptr)
        {
            const auto& patternNotes = pattern->getNotes();
            for (const auto& midiNote : patternNotes)
            {
                NoteRect noteRect;
                noteRect.note = midiNote.note;
                noteRect.startBeat = midiNote.startBeat;
                noteRect.lengthBeats = midiNote.lengthBeats;
                noteRect.bounds = noteToRect(noteRect.note, noteRect.startBeat, noteRect.lengthBeats);
                noteRect.isSelected = false;
                notes.push_back(noteRect);
            }
        }
    }

    repaint();
}

void PianoRollView::updateGhostNotes()
{
    ghostNotes.clear();

    if (!showGhostNotes || !projectModel)
        return;

    // Get clips from ghost track and extract pattern notes
    const auto* track = projectModel->getTrack(ghostTrackId);
    if (!track)
        return;

    const auto clips = projectModel->getClipsForTrack(ghostTrackId);
    for (const auto* clip : clips)
    {
        if (clip != nullptr && clip->hasPattern())
        {
            const auto* pattern = projectModel->getPattern(clip->getPatternId());
            if (pattern != nullptr)
            {
                const auto& patternNotes = pattern->getNotes();
                for (const auto& midiNote : patternNotes)
                {
                    NoteRect ghostNote;
                    ghostNote.note = midiNote.note;
                    ghostNote.startBeat = clip->getStartBeats() + midiNote.startBeat;
                    ghostNote.lengthBeats = midiNote.lengthBeats;
                    ghostNote.bounds = noteToRect(ghostNote.note, ghostNote.startBeat, ghostNote.lengthBeats);
                    ghostNote.isSelected = false;
                    ghostNotes.push_back(ghostNote);
                }
            }
        }
    }

    repaint();
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

void PianoRollView::setInferenceEngine(std::shared_ptr<daw::ai::inference::InferenceEngine> engine)
{
    inferenceEngine = engine;
    const bool enabled = engine != nullptr && engine->isReady();
    aiGenerateMelodyButton.setEnabled(enabled);
    aiGenerateChordsButton.setEnabled(enabled);
    aiGenerateGrooveButton.setEnabled(enabled);
}

void PianoRollView::setupAIButtons()
{
    using namespace daw::ui::lookandfeel::DesignSystem;

    // Style buttons with design system
    auto styleButton = [](juce::TextButton& button, const juce::String& text) {
        button.setButtonText(text);
        button.setColour(juce::TextButton::buttonColourId, toColour(Colors::surfaceElevated));
        button.setColour(juce::TextButton::buttonOnColourId, toColour(Colors::primary));
        button.setColour(juce::TextButton::textColourOnId, toColour(Colors::text));
        button.setColour(juce::TextButton::textColourOffId, toColour(Colors::textSecondary));
        button.setEnabled(false);
    };

    styleButton(aiGenerateMelodyButton, "AI Melody");
    aiGenerateMelodyButton.onClick = [this] { aiGenerateMelodyClicked(); };
    addAndMakeVisible(aiGenerateMelodyButton);

    styleButton(aiGenerateChordsButton, "AI Chords");
    aiGenerateChordsButton.onClick = [this] { aiGenerateChordsClicked(); };
    addAndMakeVisible(aiGenerateChordsButton);

    styleButton(aiGenerateGrooveButton, "AI Groove");
    aiGenerateGrooveButton.onClick = [this] { aiGenerateGrooveClicked(); };
    addAndMakeVisible(aiGenerateGrooveButton);
}

void PianoRollView::aiGenerateMelodyClicked()
{
    if (inferenceEngine == nullptr || !inferenceEngine->isReady() || isAIGenerating)
        return;

    if (projectModel == nullptr || currentPatternId == 0)
        return;

    isAIGenerating = true;
    aiGenerateMelodyButton.setEnabled(false);
    aiGenerateChordsButton.setEnabled(false);
    aiGenerateGrooveButton.setEnabled(false);

    // Get current notes as context
    const auto* pattern = projectModel->getPattern(currentPatternId);
    if (pattern == nullptr)
    {
        isAIGenerating = false;
        const bool enabled = inferenceEngine != nullptr && inferenceEngine->isReady();
        aiGenerateMelodyButton.setEnabled(enabled);
        aiGenerateChordsButton.setEnabled(enabled);
        aiGenerateGrooveButton.setEnabled(enabled);
        return;
    }

    const auto contextNotes = pattern->getNotes();

    // Use MelodyGenerator model
    std::string prompt = "Generate a melodic pattern based on these notes: ";
    for (const auto& note : contextNotes)
    {
        prompt += std::to_string(note.note) + " ";
    }
    prompt += ". Return MIDI notes in JSON format.";

    inferenceEngine->queueTextInference(prompt, [this](const std::string& result, bool success)
    {
        juce::MessageManager::callAsync([this, result, success]()
        {
            isAIGenerating = false;
            const bool enabled = inferenceEngine != nullptr && inferenceEngine->isReady();
            aiGenerateMelodyButton.setEnabled(enabled);
            aiGenerateChordsButton.setEnabled(enabled);
            aiGenerateGrooveButton.setEnabled(enabled);

            if (success && projectModel != nullptr && currentPatternId != 0)
            {
                if (parseAIMIDINotes(result))
                {
                    updateNotes();
                    repaint();
                }
            }
        });
    });
}

void PianoRollView::aiGenerateChordsClicked()
{
    if (inferenceEngine == nullptr || !inferenceEngine->isReady() || isAIGenerating)
        return;

    isAIGenerating = true;
    aiGenerateMelodyButton.setEnabled(false);
    aiGenerateChordsButton.setEnabled(false);
    aiGenerateGrooveButton.setEnabled(false);

    std::string prompt = "Generate chord progressions for a 16-bar pattern. Return MIDI notes in JSON format.";

    inferenceEngine->queueTextInference(prompt, [this](const std::string& result, bool success)
    {
        juce::MessageManager::callAsync([this, result, success]()
        {
            isAIGenerating = false;
            const bool enabled = inferenceEngine != nullptr && inferenceEngine->isReady();
            aiGenerateMelodyButton.setEnabled(enabled);
            aiGenerateChordsButton.setEnabled(enabled);
            aiGenerateGrooveButton.setEnabled(enabled);

            if (success && projectModel != nullptr && currentPatternId != 0)
            {
                if (parseAIMIDINotes(result))
                {
                    updateNotes();
                    repaint();
                }
            }
        });
    });
}

void PianoRollView::aiGenerateGrooveClicked()
{
    if (inferenceEngine == nullptr || !inferenceEngine->isReady() || isAIGenerating)
        return;

    isAIGenerating = true;
    aiGenerateMelodyButton.setEnabled(false);
    aiGenerateChordsButton.setEnabled(false);
    aiGenerateGrooveButton.setEnabled(false);

    std::string prompt = "Generate a rhythmic groove pattern with kick, snare, and hi-hat. Return MIDI notes in JSON format.";

    inferenceEngine->queueTextInference(prompt, [this](const std::string& result, bool success)
    {
        juce::MessageManager::callAsync([this, result, success]()
        {
            isAIGenerating = false;
            const bool enabled = inferenceEngine != nullptr && inferenceEngine->isReady();
            aiGenerateMelodyButton.setEnabled(enabled);
            aiGenerateChordsButton.setEnabled(enabled);
            aiGenerateGrooveButton.setEnabled(enabled);

            if (success && projectModel != nullptr && currentPatternId != 0)
            {
                if (parseAIMIDINotes(result))
                {
                    updateNotes();
                    repaint();
                }
            }
        });
    });
}

bool PianoRollView::parseAIMIDINotes(const std::string& jsonResponse)
{
    if (projectModel == nullptr || currentPatternId == 0)
        return false;

    auto* pattern = projectModel->getPattern(currentPatternId);
    if (pattern == nullptr)
        return false;

    try
    {
        // Parse JSON using JUCE's JSON parser
        juce::var parsedJson = juce::JSON::parse(juce::String(jsonResponse));

        if (!parsedJson.isObject() && !parsedJson.isArray())
        {
            // Try to extract JSON from markdown code blocks or other text
            juce::String response(jsonResponse);
            int jsonStart = response.indexOfChar('[');
            int jsonEnd = response.lastIndexOfChar(']');
            if (jsonStart < 0 || jsonEnd < 0)
            {
                jsonStart = response.indexOfChar('{');
                jsonEnd = response.lastIndexOfChar('}');
            }

            if (jsonStart >= 0 && jsonEnd > jsonStart)
            {
                response = response.substring(jsonStart, jsonEnd + 1);
                parsedJson = juce::JSON::parse(response);
            }
        }

        juce::Array<juce::var>* notesArray = nullptr;

        // Handle array of notes: [{"note": 60, "startBeat": 0.0, ...}, ...]
        if (parsedJson.isArray())
        {
            notesArray = parsedJson.getArray();
        }
        // Handle object with notes array: {"notes": [{"note": 60, ...}, ...]}
        else if (parsedJson.isObject())
        {
            auto* obj = parsedJson.getDynamicObject();
            if (obj != nullptr && obj->hasProperty("notes"))
            {
                auto notesVar = obj->getProperty("notes");
                if (notesVar.isArray())
                {
                    notesArray = notesVar.getArray();
                }
            }
        }

        if (notesArray == nullptr || notesArray->isEmpty())
            return false;

        // Clear existing notes (optional - could also append)
        // pattern->clearNotes();

        // Parse each note
        for (const auto& noteVar : *notesArray)
        {
            if (!noteVar.isObject())
                continue;

            auto* noteObj = noteVar.getDynamicObject();
            if (noteObj == nullptr)
                continue;

            daw::project::Pattern::MIDINote midiNote;

            // Helper lambdas for property access with defaults
            auto getIntProp = [&](const juce::Identifier& name, int def) {
                if (noteObj->hasProperty(name)) return static_cast<int>(noteObj->getProperty(name));
                return def;
            };
            auto getDoubleProp = [&](const juce::Identifier& name, double def) {
                if (noteObj->hasProperty(name)) return static_cast<double>(noteObj->getProperty(name));
                return def;
            };

            // Extract note properties with defaults
            midiNote.note = static_cast<uint8_t>(juce::jlimit(0, 127, getIntProp("note", 60)));
            midiNote.velocity = static_cast<uint8_t>(juce::jlimit(0, 127, getIntProp("velocity", 100)));
            midiNote.startBeat = getDoubleProp("startBeat", 0.0);
            midiNote.lengthBeats = getDoubleProp("lengthBeats", 0.25);
            midiNote.channel = static_cast<uint8_t>(juce::jlimit(0, 15, getIntProp("channel", 0)));

            // Ensure valid values
            if (midiNote.lengthBeats <= 0.0)
                midiNote.lengthBeats = 0.25; // Default to quarter note
            if (midiNote.startBeat < 0.0)
                midiNote.startBeat = 0.0;

            pattern->addNote(midiNote);
        }

        return true;
    }
    catch (...)
    {
        // JSON parsing failed - could log error here
        return false;
    }
}

} // namespace daw::ui::views

