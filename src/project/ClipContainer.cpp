#include "ClipContainer.h"

namespace daw::project
{

uint32_t ClipContainer::nextId = 1;

uint32_t ClipContainer::generateId()
{
    return nextId++;
}

ClipContainer::ClipContainer()
    : id(generateId())
    , name("Container")
    , color(juce::Colour(0xff666666))
{
}

ClipContainer::ClipContainer(const std::string& name, juce::Colour color)
    : id(generateId())
    , name(name)
    , color(color)
{
}

void ClipContainer::addClip(uint32_t clipId)
{
    if (!containsClip(clipId))
    {
        clipIds.push_back(clipId);
    }
}

void ClipContainer::removeClip(uint32_t clipId)
{
    clipIds.erase(
        std::remove(clipIds.begin(), clipIds.end(), clipId),
        clipIds.end());
}

bool ClipContainer::containsClip(uint32_t clipId) const
{
    return std::find(clipIds.begin(), clipIds.end(), clipId) != clipIds.end();
}

} // namespace daw::project

