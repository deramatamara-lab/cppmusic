#include "InspectorPanel.hpp"
#include "imgui.h"
#include <algorithm>
#include <map>

namespace daw::ui::imgui
{

InspectorPanel::InspectorPanel()
{
    // Set initial demo context
    setContext(InspectorContext::Track, "Lead Synth");
    createDemoProperties();
}

void InspectorPanel::createDemoProperties()
{
    properties_.clear();
    
    switch (context_)
    {
        case InspectorContext::Track:
            properties_ = {
                {"Name", "General", std::string("Lead Synth"), std::string(""), std::string(""), false},
                {"Color", "General", ImVec4(0.9f, 0.6f, 0.2f, 1.0f), ImVec4(0,0,0,0), ImVec4(1,1,1,1), false},
                {"Volume", "Mix", 0.8f, 0.0f, 1.0f, false},
                {"Pan", "Mix", 0.0f, -1.0f, 1.0f, false},
                {"Mute", "Mix", false, false, true, false},
                {"Solo", "Mix", false, false, true, false},
                {"Record Armed", "Recording", false, false, true, false},
                {"Input", "Recording", std::string("Audio In 1"), std::string(""), std::string(""), false},
                {"Output", "Routing", std::string("Master"), std::string(""), std::string(""), false},
                {"Height", "Display", 100, 50, 300, false},
            };
            break;
            
        case InspectorContext::Clip:
            properties_ = {
                {"Name", "General", std::string("Pattern 1"), std::string(""), std::string(""), false},
                {"Color", "General", ImVec4(0.3f, 0.6f, 0.9f, 1.0f), ImVec4(0,0,0,0), ImVec4(1,1,1,1), false},
                {"Start", "Position", 0.0, 0.0, 1000.0, false},
                {"Length", "Position", 4.0, 0.25, 256.0, false},
                {"Offset", "Position", 0.0, -16.0, 16.0, false},
                {"Gain", "Audio", 0.0f, -24.0f, 24.0f, false},
                {"Pitch", "Audio", 0, -24, 24, false},
                {"Muted", "State", false, false, true, false},
                {"Locked", "State", false, false, true, false},
            };
            break;
            
        case InspectorContext::Note:
            properties_ = {
                {"Pitch", "General", 60, 0, 127, false},
                {"Velocity", "General", 100, 0, 127, false},
                {"Start", "Position", 0.0, 0.0, 256.0, false},
                {"Length", "Position", 1.0, 0.0625, 64.0, false},
                {"Channel", "MIDI", 1, 1, 16, false},
            };
            break;
            
        case InspectorContext::Plugin:
            properties_ = {
                {"Name", "General", std::string("Reverb"), std::string(""), std::string(""), true},
                {"Bypass", "State", false, false, true, false},
                {"Mix", "Parameters", 0.3f, 0.0f, 1.0f, false},
                {"Size", "Parameters", 0.5f, 0.0f, 1.0f, false},
                {"Decay", "Parameters", 2.5f, 0.1f, 10.0f, false},
                {"Damping", "Parameters", 0.5f, 0.0f, 1.0f, false},
                {"Pre-delay", "Parameters", 20.0f, 0.0f, 200.0f, false},
            };
            break;
            
        default:
            break;
    }
}

void InspectorPanel::setContext(InspectorContext context, const std::string& name)
{
    context_ = context;
    contextName_ = name;
    createDemoProperties();
}

void InspectorPanel::setProperties(std::vector<PropertyDef> properties)
{
    properties_ = std::move(properties);
}

void InspectorPanel::draw(bool& open, const Theme& theme)
{
    if (!open) return;
    
    const auto& tokens = theme.getTokens();
    float scale = theme.getDpiScale();
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(tokens.spacingSm * scale, tokens.spacingSm * scale));
    
    if (ImGui::Begin("Inspector", &open))
    {
        if (context_ == InspectorContext::None)
        {
            ImGui::TextDisabled("No selection");
        }
        else
        {
            // Context header
            ImGui::Text("%s %s", getContextIcon(context_), getContextLabel(context_));
            ImGui::SameLine();
            ImGui::TextColored(tokens.textDisabled, "- %s", contextName_.c_str());
            
            ImGui::Separator();
            ImGui::Spacing();
            
            // Group properties by category
            std::map<std::string, std::vector<PropertyDef*>> categories;
            for (auto& prop : properties_)
            {
                categories[prop.category].push_back(&prop);
            }
            
            // Draw each category
            for (auto& [category, props] : categories)
            {
                if (ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Indent(tokens.spacingSm * scale);
                    
                    for (auto* prop : props)
                    {
                        drawProperty(*prop, theme);
                    }
                    
                    ImGui::Unindent(tokens.spacingSm * scale);
                    ImGui::Spacing();
                }
            }
        }
    }
    ImGui::End();
    
    ImGui::PopStyleVar();
}

void InspectorPanel::drawProperty(PropertyDef& prop, [[maybe_unused]] const Theme& theme)
{
    float scale = theme.getDpiScale();
    
    ImGui::PushID(prop.name.c_str());
    
    // Label on left
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", prop.name.c_str());
    ImGui::SameLine(120.0f * scale);
    
    // Value editor
    ImGui::SetNextItemWidth(-1);
    
    if (prop.readOnly)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
    }
    
    bool changed = false;
    
    std::visit([&](auto&& value) {
        using T = std::decay_t<decltype(value)>;
        
        if constexpr (std::is_same_v<T, bool>)
        {
            bool v = value;
            if (ImGui::Checkbox("##value", &v) && !prop.readOnly)
            {
                prop.value = v;
                changed = true;
            }
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            int v = value;
            int minV = std::get<int>(prop.minValue);
            int maxV = std::get<int>(prop.maxValue);
            if (ImGui::SliderInt("##value", &v, minV, maxV) && !prop.readOnly)
            {
                prop.value = v;
                changed = true;
            }
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            float v = value;
            float minV = std::get<float>(prop.minValue);
            float maxV = std::get<float>(prop.maxValue);
            if (ImGui::SliderFloat("##value", &v, minV, maxV, "%.2f") && !prop.readOnly)
            {
                prop.value = v;
                changed = true;
            }
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            float v = static_cast<float>(value);
            float minV = static_cast<float>(std::get<double>(prop.minValue));
            float maxV = static_cast<float>(std::get<double>(prop.maxValue));
            if (ImGui::SliderFloat("##value", &v, minV, maxV, "%.3f") && !prop.readOnly)
            {
                prop.value = static_cast<double>(v);
                changed = true;
            }
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            char buf[256];
            std::snprintf(buf, sizeof(buf), "%s", value.c_str());
            if (ImGui::InputText("##value", buf, sizeof(buf)) && !prop.readOnly)
            {
                prop.value = std::string(buf);
                changed = true;
            }
        }
        else if constexpr (std::is_same_v<T, ImVec4>)
        {
            ImVec4 v = value;
            if (ImGui::ColorEdit4("##value", &v.x, ImGuiColorEditFlags_NoInputs) && !prop.readOnly)
            {
                prop.value = v;
                changed = true;
            }
        }
    }, prop.value);
    
    if (prop.readOnly)
    {
        ImGui::PopStyleVar();
    }
    
    if (changed && onPropertyChanged_)
    {
        onPropertyChanged_(prop.name, prop.value);
    }
    
    ImGui::PopID();
}

const char* InspectorPanel::getContextIcon(InspectorContext ctx)
{
    switch (ctx)
    {
        case InspectorContext::Track:      return "[T]";
        case InspectorContext::Clip:       return "[C]";
        case InspectorContext::Note:       return "[N]";
        case InspectorContext::Plugin:     return "[P]";
        case InspectorContext::Automation: return "[A]";
        default:                           return "[?]";
    }
}

const char* InspectorPanel::getContextLabel(InspectorContext ctx)
{
    switch (ctx)
    {
        case InspectorContext::Track:      return "Track";
        case InspectorContext::Clip:       return "Clip";
        case InspectorContext::Note:       return "Note";
        case InspectorContext::Plugin:     return "Plugin";
        case InspectorContext::Automation: return "Automation";
        default:                           return "Unknown";
    }
}

} // namespace daw::ui::imgui
