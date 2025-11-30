#pragma once

#include "../UndoManager.h"
#include <cstdint>

namespace daw::project
{

class ProjectModel;

/**
 * @brief Command to move a container (and all its clips)
 *
 * Undoable command for moving all clips in a container by a delta.
 * Follows DAW_DEV_RULES: semantic undo steps.
 */
class MoveContainerCommand : public UndoableCommand
{
public:
    MoveContainerCommand(uint32_t containerId, double deltaBeats);

    [[nodiscard]] bool execute(ProjectModel& model) override;
    [[nodiscard]] bool undo(ProjectModel& model) override;

private:
    uint32_t containerId;
    double deltaBeats;
    std::vector<std::pair<uint32_t, double>> clipStartPositions; // clipId -> old start position
};

} // namespace daw::project

