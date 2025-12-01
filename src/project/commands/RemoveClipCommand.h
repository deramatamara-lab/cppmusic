#pragma once

#include "../UndoManager.h"
#include <cstdint>
#include <string>

namespace daw::project
{

class ProjectModel;

/**
 * @brief Command to remove a clip
 *
 * Undoable command for removing a clip from the project.
 * Follows DAW_DEV_RULES: semantic undo steps.
 */
class RemoveClipCommand : public UndoableCommand
{
public:
    explicit RemoveClipCommand(uint32_t clipId);

    [[nodiscard]] bool execute(ProjectModel& model) override;
    [[nodiscard]] bool undo(ProjectModel& model) override;

private:
    uint32_t clipId_;
    uint32_t trackId_{0};
    double startBeats_{0.0};
    double lengthBeats_{0.0};
    std::string label_;
    uint32_t patternId_{0};
};

} // namespace daw::project
