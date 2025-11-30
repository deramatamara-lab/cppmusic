#include "UpdatePatternNotesCommand.h"
#include "../ProjectModel.h"

namespace daw::project
{

UpdatePatternNotesCommand::UpdatePatternNotesCommand(uint32_t patternId,
                                                   std::vector<Pattern::MIDINote> newNotes,
                                                   std::string description)
    : UndoableCommand(std::move(description))
    , patternId(patternId)
    , newNotes(std::move(newNotes))
{
}

bool UpdatePatternNotesCommand::execute(ProjectModel& model)
{
    auto* pattern = model.getPattern(patternId);
    if (pattern == nullptr)
        return false;

    if (!capturedInitialState)
    {
        oldNotes = pattern->getNotes();
        capturedInitialState = true;
    }

    return model.setPatternNotes(patternId, newNotes);
}

bool UpdatePatternNotesCommand::undo(ProjectModel& model)
{
    auto* pattern = model.getPattern(patternId);
    if (pattern == nullptr)
        return false;

    return model.setPatternNotes(patternId, oldNotes);
}

} // namespace daw::project
