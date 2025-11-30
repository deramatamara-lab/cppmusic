#pragma once

#include "../Pattern.h"
#include "../UndoManager.h"
#include <vector>

namespace daw::project
{

class ProjectModel;

/**
 * @brief Undoable command to replace the full note set of a pattern.
 */
class UpdatePatternNotesCommand : public UndoableCommand
{
public:
    UpdatePatternNotesCommand(uint32_t patternId,
                              std::vector<Pattern::MIDINote> newNotes,
                              std::string description = "Edit Pattern Steps");

    [[nodiscard]] bool execute(ProjectModel& model) override;
    [[nodiscard]] bool undo(ProjectModel& model) override;

private:
    uint32_t patternId;
    std::vector<Pattern::MIDINote> newNotes;
    std::vector<Pattern::MIDINote> oldNotes;
    bool capturedInitialState{false};
};

} // namespace daw::project
