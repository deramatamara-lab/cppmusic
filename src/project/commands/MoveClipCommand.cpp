#include "MoveClipCommand.h"
#include "../ProjectModel.h"

namespace daw::project
{

MoveClipCommand::MoveClipCommand(uint32_t clipId, double oldStartBeats, double newStartBeats)
    : UndoableCommand("Move Clip")
    , clipId(clipId)
    , oldStartBeats(oldStartBeats)
    , newStartBeats(newStartBeats)
{
}

bool MoveClipCommand::execute(ProjectModel& model)
{
    auto* clip = model.getClip(clipId);
    if (clip == nullptr)
        return false;

    clip->setStartBeats(newStartBeats);
    return true;
}

bool MoveClipCommand::undo(ProjectModel& model)
{
    auto* clip = model.getClip(clipId);
    if (clip == nullptr)
        return false;

    clip->setStartBeats(oldStartBeats);
    return true;
}

} // namespace daw::project

