#include "TrimClipCommand.h"
#include "../ProjectModel.h"

namespace daw::project
{

TrimClipCommand::TrimClipCommand(uint32_t clipId, double oldLengthBeats, double newLengthBeats, bool trimStart)
    : UndoableCommand(trimStart ? "Trim Clip Start" : "Trim Clip End")
    , clipId(clipId)
    , oldLengthBeats(oldLengthBeats)
    , newLengthBeats(newLengthBeats)
    , oldStartBeats(0.0)
    , newStartBeats(0.0)
    , trimStart(trimStart)
{
}

bool TrimClipCommand::execute(ProjectModel& model)
{
    auto* clip = model.getClip(clipId);
    if (clip == nullptr)
        return false;

    oldStartBeats = clip->getStartBeats();

    if (trimStart)
    {
        // Trimming start: adjust both start and length
        const double lengthChange = newLengthBeats - oldLengthBeats;
        newStartBeats = oldStartBeats + lengthChange;
        clip->setStartBeats(newStartBeats);
        clip->setLengthBeats(newLengthBeats);
    }
    else
    {
        // Trimming end: only adjust length
        clip->setLengthBeats(newLengthBeats);
    }

    return true;
}

bool TrimClipCommand::undo(ProjectModel& model)
{
    auto* clip = model.getClip(clipId);
    if (clip == nullptr)
        return false;

    if (trimStart)
    {
        clip->setStartBeats(oldStartBeats);
    }
    clip->setLengthBeats(oldLengthBeats);

    return true;
}

} // namespace daw::project

