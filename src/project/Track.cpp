#include "Track.h"
#include <algorithm>

namespace daw::project
{

uint32_t Track::nextId = 1;

Track::Track(std::string name, juce::Colour color)
    : id(generateId())
    , name(std::move(name))
    , color(color)
{
}

uint32_t Track::generateId()
{
    return nextId++;
}

} // namespace daw::project

