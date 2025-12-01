#pragma once

#include "../Theme.hpp"
#include <string>
#include <vector>
#include <variant>
#include <functional>

namespace daw::ui::imgui
{

/**
 * @brief Property types for inspector
 */
using PropertyValue = std::variant<
    bool,
    int,
    float,
    double,
    std::string,
    ImVec4
>;

/**
 * @brief Property definition for inspector
 */
struct PropertyDef
{
    std::string name;
    std::string category;
    PropertyValue value;
    PropertyValue minValue;
    PropertyValue maxValue;
    bool readOnly{false};
};

/**
 * @brief Inspector context for showing different object types
 */
enum class InspectorContext
{
    None,
    Track,
    Clip,
    Note,
    Plugin,
    Automation
};

/**
 * @brief Inspector panel for context-sensitive properties
 * 
 * Features:
 * - Context-sensitive property display
 * - Categorized properties
 * - Various property editors (sliders, color pickers, etc.)
 */
class InspectorPanel
{
public:
    InspectorPanel();
    ~InspectorPanel() = default;

    /**
     * @brief Draw the inspector panel
     * @param open Reference to visibility flag
     * @param theme Theme for styling
     */
    void draw(bool& open, const Theme& theme);

    /**
     * @brief Set current context
     */
    void setContext(InspectorContext context, const std::string& name = "");

    /**
     * @brief Set properties to display
     */
    void setProperties(std::vector<PropertyDef> properties);

    /**
     * @brief Set callback for property changes
     */
    void setOnPropertyChanged(std::function<void(const std::string&, const PropertyValue&)> callback)
    {
        onPropertyChanged_ = std::move(callback);
    }

private:
    InspectorContext context_{InspectorContext::None};
    std::string contextName_;
    std::vector<PropertyDef> properties_;
    std::function<void(const std::string&, const PropertyValue&)> onPropertyChanged_;

    void drawProperty(PropertyDef& prop, const Theme& theme);
    void createDemoProperties();
    
    static const char* getContextIcon(InspectorContext ctx);
    static const char* getContextLabel(InspectorContext ctx);
};

} // namespace daw::ui::imgui
