#include "Clip.h"

namespace daw::project
{

uint32_t Clip::nextId = 1;

Clip::Clip(uint32_t trackId, double startBeats, double lengthBeats, std::string label)
    : id(generateId())
    , trackId(trackId)
    , startBeats(startBeats)
    , lengthBeats(lengthBeats)
    , label(std::move(label))
{
}

uint32_t Clip::generateId()
{
    return nextId++;
}

} // namespace daw::project

