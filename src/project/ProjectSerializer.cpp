#include "ProjectSerializer.h"
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
                clipsArray.add(clipVar);
            }
        }
    }
    root->setProperty("clips", clipsArray);
    
    return juce::JSON::toString(rootVar).toStdString();
}

std::unique_ptr<ProjectModel> ProjectSerializer::deserialize(const std::string& json) const
{
    auto model = std::make_unique<ProjectModel>();
    
    const auto parsed = juce::JSON::parse(json);
    if (!parsed.isObject())
        return nullptr;
    
    const auto root = parsed.getDynamicObject();
    if (!root)
        return nullptr;
    
    const auto version = root->getProperty("version");
    if (version.isInt())
    {
        const auto versionNum = version.operator int();
        // TODO: Handle version migration if needed
        juce::ignoreUnused(versionNum);
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

