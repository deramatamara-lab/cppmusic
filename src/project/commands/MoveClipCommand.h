#pragma once

#include "../UndoManager.h"
#include <cstdint>

namespace daw::project
{

class ProjectModel;

/**
 * @brief Command to move a clip
 *
 * Undoable command for moving a clip to a new position.
 * Follows DAW_DEV_RULES: semantic undo steps.
 */
class MoveClipCommand : public UndoableCommand
{
public:
    MoveClipCommand(uint32_t clipId, double oldStartBeats, double newStartBeats);

    [[nodiscard]] bool execute(ProjectModel& model) override;
    [[nodiscard]] bool undo(ProjectModel& model) override;

private:
    uint32_t clipId;
    double oldStartBeats;
    double newStartBeats;
};

} // namespace daw::project

