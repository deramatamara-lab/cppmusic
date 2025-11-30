#include "Pattern.h"
#include <algorithm>
#include <cmath>

namespace daw::project
{

uint32_t Pattern::nextId = 1;

uint32_t Pattern::generateId()
{
    return nextId++;
}

Pattern::Pattern() noexcept
    : id(generateId())
    , name("Untitled Pattern")
    , numSteps(16)
{
}

Pattern::Pattern(const std::string& name, int numSteps) noexcept
    : id(generateId())
    , name(name)
    , numSteps(numSteps)
{
}

void Pattern::addNote(const MIDINote& note) noexcept
{
    notes.push_back(note);
    std::sort(notes.begin(), notes.end(),
        [](const MIDINote& a, const MIDINote& b) { return a.startBeat < b.startBeat; });
}

void Pattern::removeNote(size_t index) noexcept
{
    if (index < notes.size())
    {
        notes.erase(notes.begin() + static_cast<ptrdiff_t>(index));
    }
}

void Pattern::clearNotes() noexcept
{
    notes.clear();
}

void Pattern::setNotes(const std::vector<MIDINote>& newNotes) noexcept
{
    notes = newNotes;
    std::sort(notes.begin(), notes.end(),
        [](const MIDINote& a, const MIDINote& b) { return a.startBeat < b.startBeat; });
}

std::vector<Pattern::MIDINote> Pattern::getNotesForStep(int step) const noexcept
{
    std::vector<MIDINote> stepNotes;

    const auto stepStart = static_cast<double>(step);
    const auto stepEnd = static_cast<double>(step + 1);

    for (const auto& note : notes)
    {
        if (note.startBeat >= stepStart && note.startBeat < stepEnd)
        {
            stepNotes.push_back(note);
        }
    }

    return stepNotes;
}

void Pattern::quantize(double gridDivision) noexcept
{
    for (auto& note : notes)
    {
        const auto quantized = std::round(note.startBeat / gridDivision) * gridDivision;
        note.startBeat = quantized;
    }

    // Re-sort after quantization
    std::sort(notes.begin(), notes.end(),
        [](const MIDINote& a, const MIDINote& b) { return a.startBeat < b.startBeat; });
}

double Pattern::getLengthBeats() const noexcept
{
    if (notes.empty())
        return static_cast<double>(numSteps);

    double maxEnd = 0.0;
    for (const auto& note : notes)
    {
        const auto endBeat = note.startBeat + note.lengthBeats;
        if (endBeat > maxEnd)
            maxEnd = endBeat;
    }

    return std::max(maxEnd, static_cast<double>(numSteps));
}

} // namespace daw::project

