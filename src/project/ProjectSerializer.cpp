#include "ProjectSerializer.h"
#include "ProjectMigrator.h"
#include <juce_core/juce_core.h>
#include <fstream>
#include <sstream>

namespace daw::project
{

std::string ProjectSerializer::serialize(const ProjectModel& model) const
{
    juce::var rootVar{ new juce::DynamicObject() };
    auto* root = rootVar.getDynamicObject();
    jassert(root != nullptr);
    root->setProperty("version", CURRENT_VERSION);
    root->setProperty("name", "Untitled Project");

    // Serialize tracks
    juce::Array<juce::var> tracksArray;
    const auto tracks = model.getTracks();
    for (const auto* track : tracks)
    {
        if (track)
        {
            juce::var trackVar{ new juce::DynamicObject() };
            if (auto* trackObj = trackVar.getDynamicObject())
            {
                trackObj->setProperty("id", static_cast<int>(track->getId()));
                trackObj->setProperty("name", juce::String(track->getName()));
                trackObj->setProperty("color", track->getColor().toString());
                trackObj->setProperty("gainDb", track->getGainDb());
                trackObj->setProperty("pan", track->getPan());
                trackObj->setProperty("muted", track->isMuted());
                trackObj->setProperty("soloed", track->isSoloed());
                tracksArray.add(trackVar);
            }
        }
    }
    root->setProperty("tracks", tracksArray);

    // Serialize clips
    juce::Array<juce::var> clipsArray;
    const auto clips = model.getClips();
    for (const auto* clip : clips)
    {
        if (clip)
        {
            juce::var clipVar{ new juce::DynamicObject() };
            if (auto* clipObj = clipVar.getDynamicObject())
            {
                clipObj->setProperty("id", static_cast<int>(clip->getId()));
                clipObj->setProperty("trackId", static_cast<int>(clip->getTrackId()));
                clipObj->setProperty("startBeats", clip->getStartBeats());
                clipObj->setProperty("lengthBeats", clip->getLengthBeats());
                clipObj->setProperty("label", juce::String(clip->getLabel()));
                if (clip->hasPattern())
                    clipObj->setProperty("patternId", static_cast<int>(clip->getPatternId()));
                clipsArray.add(clipVar);
            }
        }
    }
    root->setProperty("clips", clipsArray);

    // Serialize patterns
    juce::Array<juce::var> patternsArray;
    const auto patterns = model.getPatterns();
    for (const auto* pattern : patterns)
    {
        if (pattern)
        {
            juce::var patternVar{ new juce::DynamicObject() };
            if (auto* patternObj = patternVar.getDynamicObject())
            {
                patternObj->setProperty("id", static_cast<int>(pattern->getId()));
                patternObj->setProperty("name", juce::String(pattern->getName()));
                patternObj->setProperty("numSteps", pattern->getNumSteps());

                // Serialize MIDI notes
                juce::Array<juce::var> notesArray;
                const auto& notes = pattern->getNotes();
                for (const auto& note : notes)
                {
                    juce::var noteVar{ new juce::DynamicObject() };
                    if (auto* noteObj = noteVar.getDynamicObject())
                    {
                        noteObj->setProperty("note", note.note);
                        noteObj->setProperty("velocity", note.velocity);
                        noteObj->setProperty("startBeat", note.startBeat);
                        noteObj->setProperty("lengthBeats", note.lengthBeats);
                        noteObj->setProperty("channel", note.channel);
                        noteObj->setProperty("probability", note.probability);
                        noteObj->setProperty("microTiming", note.microTiming);
                        noteObj->setProperty("trigCondition", note.trigCondition);
                        notesArray.add(noteVar);
                    }
                }
                patternObj->setProperty("notes", notesArray);
                patternsArray.add(patternVar);
            }
        }
    }
    root->setProperty("patterns", patternsArray);

    // Serialize pattern-clip associations
    juce::Array<juce::var> associationsArray;
    for (const auto* clip : clips)
    {
        if (clip && clip->hasPattern())
        {
            juce::var assocVar{ new juce::DynamicObject() };
            if (auto* assocObj = assocVar.getDynamicObject())
            {
                assocObj->setProperty("clipId", static_cast<int>(clip->getId()));
                assocObj->setProperty("patternId", static_cast<int>(clip->getPatternId()));
                associationsArray.add(assocVar);
            }
        }
    }
    root->setProperty("patternAssociations", associationsArray);

    return juce::JSON::toString(rootVar).toStdString();
}

std::unique_ptr<ProjectModel> ProjectSerializer::deserialize(const std::string& json) const
{
    auto model = std::make_unique<ProjectModel>();

    const auto parsed = juce::JSON::parse(json);
    if (!parsed.isObject())
        return nullptr;

    auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return nullptr;

    const auto version = root->getProperty("version");
    int versionNum = 1; // Default to current version
    if (version.isInt())
    {
        versionNum = version.operator int();
    }

    // Handle version migration
    if (ProjectMigrator::needsMigration(versionNum))
    {
        ProjectMigrator migrator;
        const auto migratedJson = migrator.migrate(json, versionNum);
        if (!migratedJson.empty())
        {
            // Re-parse migrated JSON
            const auto migratedParsed = juce::JSON::parse(migratedJson);
            if (migratedParsed.isObject())
            {
                const auto migratedRoot = migratedParsed.getDynamicObject();
                if (migratedRoot)
                {
                    root = migratedRoot;
                }
            }
        }
    }

    // Deserialize tracks
    const auto tracksVar = root->getProperty("tracks");
    if (tracksVar.isArray())
    {
        const auto tracksArray = tracksVar.getArray();
        for (const auto& trackVar : *tracksArray)
        {
            if (trackVar.isObject())
            {
                const auto trackObj = trackVar.getDynamicObject();
                if (trackObj)
                {
                    const auto name = trackObj->getProperty("name").toString().toStdString();
                    const auto colorStr = trackObj->getProperty("color").toString();
                    const auto color = juce::Colour::fromString(colorStr);
                    model->addTrack(name, color);
                }
            }
        }
    }

    // Deserialize clips
    const auto clipsVar = root->getProperty("clips");
    if (clipsVar.isArray())
    {
        const auto clipsArray = clipsVar.getArray();
        for (const auto& clipVar : *clipsArray)
        {
            if (clipVar.isObject())
            {
                const auto clipObj = clipVar.getDynamicObject();
                if (clipObj)
                {
                    const auto trackId = static_cast<uint32_t>(clipObj->getProperty("trackId").operator int());
                    const auto startBeats = clipObj->getProperty("startBeats").operator double();
                    const auto lengthBeats = clipObj->getProperty("lengthBeats").operator double();
                    const auto label = clipObj->getProperty("label").toString().toStdString();
                    model->addClip(trackId, startBeats, lengthBeats, label);
                }
            }
        }
    }

    // Deserialize patterns
    const auto patternsVar = root->getProperty("patterns");
    if (patternsVar.isArray())
    {
        const auto patternsArray = patternsVar.getArray();
        for (const auto& patternVar : *patternsArray)
        {
            if (patternVar.isObject())
            {
                const auto patternObj = patternVar.getDynamicObject();
                if (patternObj)
                {
                    const auto name = patternObj->getProperty("name").toString().toStdString();
                    const auto numSteps = patternObj->getProperty("numSteps").operator int();
                    auto* pattern = model->addPattern(name, numSteps);

                    if (pattern)
                    {
                        // Deserialize MIDI notes
                        const auto notesVar = patternObj->getProperty("notes");
                        if (notesVar.isArray())
                        {
                            const auto notesArray = notesVar.getArray();
                            for (const auto& noteVar : *notesArray)
                            {
                                if (noteVar.isObject())
                                {
                                    const auto noteObj = noteVar.getDynamicObject();
                                    if (noteObj)
                                    {
                                        Pattern::MIDINote note;
                                        note.note = noteObj->getProperty("note").operator int();
                                        note.velocity = static_cast<uint8_t>(noteObj->getProperty("velocity").operator int());
                                        note.startBeat = noteObj->getProperty("startBeat").operator double();
                                        note.lengthBeats = noteObj->getProperty("lengthBeats").operator double();
                                        note.channel = static_cast<uint8_t>(noteObj->getProperty("channel").operator int());
                                        note.probability = noteObj->hasProperty("probability")
                                                              ? static_cast<float>(noteObj->getProperty("probability").operator double())
                                                              : 1.0f;
                                        note.microTiming = noteObj->hasProperty("microTiming")
                                                              ? static_cast<float>(noteObj->getProperty("microTiming").operator double())
                                                              : 0.0f;
                                        note.trigCondition = noteObj->hasProperty("trigCondition")
                                                                ? noteObj->getProperty("trigCondition").operator int()
                                                                : 0;
                                        pattern->addNote(note);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Deserialize pattern-clip associations
    const auto associationsVar = root->getProperty("patternAssociations");
    if (associationsVar.isArray())
    {
        const auto associationsArray = associationsVar.getArray();
        for (const auto& assocVar : *associationsArray)
        {
            if (assocVar.isObject())
            {
                const auto assocObj = assocVar.getDynamicObject();
                if (assocObj)
                {
                    const auto clipId = static_cast<uint32_t>(assocObj->getProperty("clipId").operator int());
                    const auto patternId = static_cast<uint32_t>(assocObj->getProperty("patternId").operator int());
                    model->linkClipToPattern(clipId, patternId);
                }
            }
        }
    }

    return model;
}

bool ProjectSerializer::saveToFile(const ProjectModel& model, const std::string& filePath) const
{
    try
    {
        const auto json = serialize(model);
        std::ofstream file(filePath);
        if (!file.is_open())
            return false;

        file << json;
        file.close();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::unique_ptr<ProjectModel> ProjectSerializer::loadFromFile(const std::string& filePath) const
{
    try
    {
        std::ifstream file(filePath);
        if (!file.is_open())
            return nullptr;

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        return deserialize(buffer.str());
    }
    catch (...)
    {
        return nullptr;
    }
}

int ProjectSerializer::getVersionFromFile(const std::string& filePath) const
{
    try
    {
        std::ifstream file(filePath);
        if (!file.is_open())
            return -1;

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        const auto parsed = juce::JSON::parse(buffer.str());
        if (parsed.isObject())
        {
            const auto root = parsed.getDynamicObject();
            if (root)
            {
                const auto version = root->getProperty("version");
                if (version.isInt())
                    return version.operator int();
            }
        }

        return -1;
    }
    catch (...)
    {
        return -1;
    }
}

std::string ProjectSerializer::serializeTrack(const Track& track) const
{
    juce::ignoreUnused(track);
    // Implementation would serialize track details
    return "{}";
}

std::string ProjectSerializer::serializeClip(const Clip& clip) const
{
    juce::ignoreUnused(clip);
    // Implementation would serialize clip details
    return "{}";
}

Track* ProjectSerializer::deserializeTrack(const std::string& json) const
{
    juce::ignoreUnused(json);
    // Implementation would deserialize track
    return nullptr;
}

Clip* ProjectSerializer::deserializeClip(const std::string& json) const
{
    juce::ignoreUnused(json);
    // Implementation would deserialize clip
    return nullptr;
}

} // namespace daw::project

