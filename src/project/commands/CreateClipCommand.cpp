#include "CreateClipCommand.h"
#include "../ProjectModel.h"

namespace daw::project
{

CreateClipCommand::CreateClipCommand(uint32_t trackId, double startBeats, double lengthBeats, const std::string& label)
    : UndoableCommand("Create Clip")
    , trackId(trackId)
    , startBeats(startBeats)
    , lengthBeats(lengthBeats)
    , label(label)
{
}

bool CreateClipCommand::execute(ProjectModel& model)
{
    auto* clip = model.addClip(trackId, startBeats, lengthBeats, label);
    if (clip == nullptr)
        return false;

    createdClipId = clip->getId();
    return true;
}

bool CreateClipCommand::undo(ProjectModel& model)
{
    if (createdClipId == 0)
        return false;

    model.removeClip(createdClipId);
    createdClipId = 0;
    return true;
}

} // namespace daw::project

