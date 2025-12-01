#include "RemoveClipCommand.h"
#include "../ProjectModel.h"

namespace daw::project
{

RemoveClipCommand::RemoveClipCommand(uint32_t clipIdParam)
    : clipId(clipIdParam)
{
}

bool RemoveClipCommand::execute(ProjectModel& /*model*/)
{
    // Stub implementation - actual removal logic to be implemented
    return true;
}

bool RemoveClipCommand::undo(ProjectModel& /*model*/)
{
    // Stub implementation - actual undo logic to be implemented
    return true;
}

} // namespace daw::project
