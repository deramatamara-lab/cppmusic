#include "GroupClipsCommand.h"
#include "../ProjectModel.h"

namespace daw::project
{

GroupClipsCommand::GroupClipsCommand(const std::vector<uint32_t>& clipIds, const std::string& name, juce::Colour color)
    : UndoableCommand("Group Clips")
    , clipIds(clipIds)
    , name(name)
    , color(color)
{
}

bool GroupClipsCommand::execute(ProjectModel& model)
{
    if (clipIds.empty())
        return false;

    auto* container = model.addContainer(name, color);
    if (container == nullptr)
        return false;

    createdContainerId = container->getId();

    for (const auto clipId : clipIds)
    {
        container->addClip(clipId);
    }

    return true;
}

bool GroupClipsCommand::undo(ProjectModel& model)
{
    if (createdContainerId == 0)
        return false;

    model.removeContainer(createdContainerId);
    createdContainerId = 0;
    return true;
}

} // namespace daw::project

