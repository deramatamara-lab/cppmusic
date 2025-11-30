#include "PatternJsonParser.h"
#include <algorithm>
#include <cmath>

namespace daw::project
{

namespace
{
    static Pattern::MIDINote parseNoteObject(const juce::var& noteVar,
                                             int& inferredMaxStep,
                                             double& inferredMaxBeatEnd)
    {
        Pattern::MIDINote note{};
        note.note = 60;
        note.velocity = 100;
        note.startBeat = 0.0;
        note.lengthBeats = 0.25;
        note.channel = 0;
        note.probability = 1.0f;
        note.microTiming = 0.0f;
        note.trigCondition = 0;

        if (auto* obj = noteVar.getDynamicObject())
        {
            if (!obj->getProperty("note").isVoid())
                note.note = static_cast<uint8_t>(juce::jlimit(0, 127, static_cast<int>(obj->getProperty("note"))));
            else if (!obj->getProperty("pitch").isVoid())
                note.note = static_cast<uint8_t>(juce::jlimit(0, 127, static_cast<int>(obj->getProperty("pitch"))));

            if (!obj->getProperty("velocity").isVoid())
                note.velocity = static_cast<uint8_t>(juce::jlimit(1, 127, static_cast<int>(obj->getProperty("velocity"))));
            else if (!obj->getProperty("vel").isVoid())
                note.velocity = static_cast<uint8_t>(juce::jlimit(1, 127, static_cast<int>(obj->getProperty("vel"))));

            double startBeat = 0.0;
            bool haveStart = false;

            if (!obj->getProperty("step").isVoid())
            {
                const int step = juce::jmax(0, static_cast<int>(obj->getProperty("step")));
                startBeat = static_cast<double>(step);
                inferredMaxStep = juce::jmax(inferredMaxStep, step);
                haveStart = true;
            }

            if (!haveStart)
            {
                if (!obj->getProperty("beat").isVoid())
                {
                    startBeat = juce::jmax(0.0, static_cast<double>(obj->getProperty("beat")));
                    haveStart = true;
                }
                else if (!obj->getProperty("startBeat").isVoid())
                {
                    startBeat = juce::jmax(0.0, static_cast<double>(obj->getProperty("startBeat")));
                    haveStart = true;
                }
            }

            if (haveStart)
                note.startBeat = startBeat;

            if (!obj->getProperty("length").isVoid())
                note.lengthBeats = juce::jlimit(0.01, 32.0, static_cast<double>(obj->getProperty("length")));
            else if (!obj->getProperty("lengthBeats").isVoid())
                note.lengthBeats = juce::jlimit(0.01, 32.0, static_cast<double>(obj->getProperty("lengthBeats")));

            if (!obj->getProperty("channel").isVoid())
                note.channel = static_cast<uint8_t>(juce::jlimit(0, 15, static_cast<int>(obj->getProperty("channel"))));

            if (!obj->getProperty("probability").isVoid())
                note.probability = juce::jlimit(0.0f, 1.0f, static_cast<float>(obj->getProperty("probability")));
            else if (!obj->getProperty("prob").isVoid())
                note.probability = juce::jlimit(0.0f, 1.0f, static_cast<float>(obj->getProperty("prob")));

            if (!obj->getProperty("microTiming").isVoid())
                note.microTiming = juce::jlimit(-1.0f, 1.0f, static_cast<float>(obj->getProperty("microTiming")));

            if (!obj->getProperty("trigCondition").isVoid())
                note.trigCondition = static_cast<int>(obj->getProperty("trigCondition"));

            const double endBeat = note.startBeat + note.lengthBeats;
            inferredMaxBeatEnd = juce::jmax(inferredMaxBeatEnd, endBeat);
        }

        return note;
    }
}

bool parsePatternFromJson(const juce::String& jsonText, ParsedPatternFromJson& outPattern) noexcept
{
    outPattern.notes.clear();
    outPattern.numSteps = 0;

    const juce::var parsed = juce::JSON::parse(jsonText);
    if (parsed.isVoid())
        return false;

    int declaredSteps = 0;
    int inferredMaxStep = -1;
    double inferredMaxBeatEnd = 0.0;

    auto parseFromObject = [&](const juce::var& objVar) -> bool
    {
        if (!objVar.isObject())
            return false;

        if (auto* root = objVar.getDynamicObject())
        {
            if (!root->getProperty("steps").isVoid())
                declaredSteps = juce::jlimit(1, 512, static_cast<int>(root->getProperty("steps")));

            const auto notesVar = root->getProperty("notes");
            if (!notesVar.isArray())
                return false;

            const auto* arr = notesVar.getArray();
            if (arr == nullptr)
                return false;

            for (const auto& element : *arr)
            {
                outPattern.notes.push_back(parseNoteObject(element, inferredMaxStep, inferredMaxBeatEnd));
            }

            return !outPattern.notes.empty();
        }
        return false;
    };

    auto parseFromArray = [&](const juce::var& arrayVar) -> bool
    {
        if (!arrayVar.isArray())
            return false;

        const auto* arr = arrayVar.getArray();
        if (arr == nullptr || arr->isEmpty())
            return false;

        for (const auto& element : *arr)
        {
            outPattern.notes.push_back(parseNoteObject(element, inferredMaxStep, inferredMaxBeatEnd));
        }

        return !outPattern.notes.empty();
    };

    bool ok = false;
    if (parsed.isObject())
        ok = parseFromObject(parsed);
    else if (parsed.isArray())
        ok = parseFromArray(parsed);

    if (!ok || outPattern.notes.empty())
    {
        outPattern.notes.clear();
        outPattern.numSteps = 0;
        return false;
    }

    const int inferredFromSteps = (inferredMaxStep >= 0) ? (inferredMaxStep + 1) : 0;
    const int inferredFromBeats = (inferredMaxBeatEnd > 0.0)
        ? static_cast<int>(std::ceil(inferredMaxBeatEnd))
        : 0;

    int numSteps = declaredSteps;
    numSteps = juce::jmax(numSteps, inferredFromSteps);
    numSteps = juce::jmax(numSteps, inferredFromBeats);
    numSteps = juce::jlimit(1, 512, numSteps);

    outPattern.numSteps = numSteps;

    std::sort(outPattern.notes.begin(), outPattern.notes.end(),
              [](const Pattern::MIDINote& a, const Pattern::MIDINote& b)
              {
                  return a.startBeat < b.startBeat;
              });

    return true;
}

} // namespace daw::project
