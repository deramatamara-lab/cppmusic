#pragma once

#include <vector>
#include <juce_core/juce_core.h>
#include "Pattern.h"

namespace daw::project
{

struct ParsedPatternFromJson
{
    int numSteps{0};
    std::vector<Pattern::MIDINote> notes;
};

bool parsePatternFromJson(const juce::String& jsonText, ParsedPatternFromJson& outPattern) noexcept;

} // namespace daw::project
