#include "MoveContainerCommand.h"
#include "../ProjectModel.h"
#include "../ClipContainer.h"

namespace daw::project
{

MoveContainerCommand::MoveContainerCommand(uint32_t containerId, double deltaBeats)
    : UndoableCommand("Move Container")
    , containerId(containerId)
    , deltaBeats(deltaBeats)
{
}

bool MoveContainerCommand::execute(ProjectModel& model)
{
    auto* container = model.getContainer(containerId);
    if (container == nullptr)
        return false;

    // Store old positions
    clipStartPositions.clear();
    for (const auto clipId : container->getClips())
    {
        auto* clip = model.getClip(clipId);
        if (clip != nullptr)
        {
            clipStartPositions.emplace_back(clipId, clip->getStartBeats());
            clip->setStartBeats(clip->getStartBeats() + deltaBeats);
        }
    }

    return true;
}

bool MoveContainerCommand::undo(ProjectModel& model)
{
    // Restore old positions
    for (const auto& [clipId, oldStart] : clipStartPositions)
    {
        auto* clip = model.getClip(clipId);
        if (clip != nullptr)
        {
            clip->setStartBeats(oldStart);
        }
    }

    return true;
}

} // namespace daw::project

