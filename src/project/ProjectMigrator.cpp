#include "ProjectMigrator.h"
#include "ProjectSerializer.h"
#include <juce_core/juce_core.h>

namespace daw::project
{

std::string ProjectMigrator::migrate(const std::string& json, int fromVersion) const
{
    if (fromVersion >= ProjectSerializer::CURRENT_VERSION)
        return json; // No migration needed

    std::string currentJson = json;

    // Apply migrations sequentially
    for (int version = fromVersion; version < ProjectSerializer::CURRENT_VERSION; ++version)
    {
        switch (version)
        {
            case 0:
                currentJson = migrateV0ToV1(currentJson);
                break;
            default:
                // Unknown version, return empty
                return "";
        }
    }

    return currentJson;
}

std::string ProjectMigrator::migrateV0ToV1(const std::string& json) const
{
    // Migration from version 0 to version 1
    // Version 1 adds: patterns, pattern associations, track mixer parameters

    const auto parsed = juce::JSON::parse(json);
    if (!parsed.isObject())
        return "";

    auto root = parsed.getDynamicObject();
    if (!root)
        return "";

    // Update version
    root->setProperty("version", 1);

    // Ensure tracks have mixer parameters (add defaults if missing)
    const auto tracksVar = root->getProperty("tracks");
    if (tracksVar.isArray())
    {
        const auto tracksArray = tracksVar.getArray();
        for (auto& trackVar : *tracksArray)
        {
            if (trackVar.isObject())
            {
                auto* trackObj = trackVar.getDynamicObject();
                if (trackObj)
                {
                    // Add default mixer parameters if missing
                    if (!trackObj->hasProperty("gainDb"))
                        trackObj->setProperty("gainDb", 0.0);
                    if (!trackObj->hasProperty("pan"))
                        trackObj->setProperty("pan", 0.0);
                    if (!trackObj->hasProperty("muted"))
                        trackObj->setProperty("muted", false);
                    if (!trackObj->hasProperty("soloed"))
                        trackObj->setProperty("soloed", false);
                }
            }
        }
    }

    // Ensure clips have patternId property (default to 0 if missing)
    const auto clipsVar = root->getProperty("clips");
    if (clipsVar.isArray())
    {
        const auto clipsArray = clipsVar.getArray();
        for (auto& clipVar : *clipsArray)
        {
            if (clipVar.isObject())
            {
                auto* clipObj = clipVar.getDynamicObject();
                if (clipObj && !clipObj->hasProperty("patternId"))
                {
                    clipObj->setProperty("patternId", 0); // No pattern
                }
            }
        }
    }

    // Add empty patterns array if missing
    if (!root->hasProperty("patterns"))
    {
        root->setProperty("patterns", juce::Array<juce::var>());
    }

    // Add empty pattern associations array if missing
    if (!root->hasProperty("patternAssociations"))
    {
        root->setProperty("patternAssociations", juce::Array<juce::var>());
    }

    return juce::JSON::toString(parsed).toStdString();
}

} // namespace daw::project

