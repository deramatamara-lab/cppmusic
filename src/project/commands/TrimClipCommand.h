#pragma once


#include "../UndoManager.h"
#include <cstdint>

namespace daw::project
{

class ProjectModel;

/**
 * @brief Command to trim a clip
 *
 * Undoable command for trimming a clip (changing its length).
 * Follows DAW_DEV_RULES: semantic undo steps.
 */
class TrimClipCommand : public UndoableCommand
{
public:
    TrimClipCommand(uint32_t clipId, double oldLengthBeats, double newLengthBeats, bool trimStart);

    [[nodiscard]] bool execute(ProjectModel& model) override;
    [[nodiscard]] bool undo(ProjectModel& model) override;

private:
    uint32_t clipId;
    double oldLengthBeats;
    double newLengthBeats;
    double oldStartBeats;
    double newStartBeats;
    bool trimStart; // true = trim start (left edge), false = trim end (right edge)
};

} // namespace daw::project

