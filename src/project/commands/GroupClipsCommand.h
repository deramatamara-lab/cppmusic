#pragma once

#include "../UndoManager.h"
#include <vector>
#include <cstdint>
#include <juce_graphics/juce_graphics.h>

namespace daw::project
{

class ProjectModel;

/**
 * @brief Command to group clips into a container
 *
 * Undoable command for creating a container and adding clips to it.
 * Follows DAW_DEV_RULES: semantic undo steps.
 */
class GroupClipsCommand : public UndoableCommand
{
public:
    GroupClipsCommand(const std::vector<uint32_t>& clipIds, const std::string& name, juce::Colour color);

    [[nodiscard]] bool execute(ProjectModel& model) override;
    [[nodiscard]] bool undo(ProjectModel& model) override;

    [[nodiscard]] uint32_t getCreatedContainerId() const { return createdContainerId; }

private:
    std::vector<uint32_t> clipIds;
    std::string name;
    juce::Colour color;
    uint32_t createdContainerId = 0;
};

} // namespace daw::project

