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
    // This would handle any format changes between versions
    
    const auto parsed = juce::JSON::parse(json);
    if (!parsed.isObject())
        return "";
    
    auto root = parsed.getDynamicObject();
    if (!root)
        return "";
    
    // Add version property
    root->setProperty("version", 1);
    
    // TODO: Apply any other migration logic needed
    
    return juce::JSON::toString(parsed).toStdString();
}

} // namespace daw::project

