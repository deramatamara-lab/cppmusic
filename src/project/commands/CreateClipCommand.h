#pragma once

#include "../UndoManager.h"
#include <cstdint>
#include <string>

namespace daw::project
{

class ProjectModel;

/**
 * @brief Command to create a clip
 *
 * Undoable command for creating a new clip.
 * Follows DAW_DEV_RULES: semantic undo steps.
 */
class CreateClipCommand : public UndoableCommand
{
public:
    CreateClipCommand(uint32_t trackId, double startBeats, double lengthBeats, const std::string& label);

    [[nodiscard]] bool execute(ProjectModel& model) override;
    [[nodiscard]] bool undo(ProjectModel& model) override;

    [[nodiscard]] uint32_t getCreatedClipId() const { return createdClipId; }

private:
    uint32_t trackId;
    double startBeats;
    double lengthBeats;
    std::string label;
    uint32_t createdClipId = 0;
};

} // namespace daw::project

