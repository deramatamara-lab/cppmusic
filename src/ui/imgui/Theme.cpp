#include "Theme.hpp"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cctype>
#include <algorithm>

namespace daw::ui::imgui
{

Theme::Theme()
{
    applyDefaultTokens();
}

void Theme::applyDefaultTokens()
{
    tokens_ = ThemeTokens{};
}

void Theme::setDpiScale(float scale)
{
    dpiScale_ = std::max(0.5f, std::min(4.0f, scale));
}

float Theme::spacing(int level) const
{
    switch (level)
    {
        case 0: return tokens_.spacingXs * dpiScale_;
        case 1: return tokens_.spacingSm * dpiScale_;
        case 2: return tokens_.spacingMd * dpiScale_;
        case 3: return tokens_.spacingLg * dpiScale_;
        case 4: return tokens_.spacingXl * dpiScale_;
        default: return tokens_.spacingMd * dpiScale_;
    }
}

void Theme::applyToImGui()
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Apply DPI scaling to all sizes
    const float s = dpiScale_;

    // Window & Frame
    style.WindowPadding = ImVec2(tokens_.spacingSm * s, tokens_.spacingSm * s);
    style.FramePadding = ImVec2(tokens_.spacingSm * s, tokens_.spacingXs * s);
    style.CellPadding = ImVec2(tokens_.spacingXs * s, tokens_.spacingXs * s);
    style.ItemSpacing = ImVec2(tokens_.spacingSm * s, tokens_.spacingXs * s);
    style.ItemInnerSpacing = ImVec2(tokens_.spacingXs * s, tokens_.spacingXs * s);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing = tokens_.spacingMd * s;
    style.ScrollbarSize = tokens_.scrollbarSize * s;
    style.GrabMinSize = tokens_.grabMinSize * s;

    // Borders
    style.WindowBorderSize = tokens_.borderWidth;
    style.ChildBorderSize = tokens_.borderWidth;
    style.PopupBorderSize = tokens_.borderWidth;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    // Rounding
    style.WindowRounding = tokens_.radiusMd * s;
    style.ChildRounding = tokens_.radiusSm * s;
    style.FrameRounding = tokens_.radiusSm * s;
    style.PopupRounding = tokens_.radiusMd * s;
    style.ScrollbarRounding = tokens_.radiusSm * s;
    style.GrabRounding = tokens_.radiusSm * s;
    style.TabRounding = tokens_.radiusSm * s;

    // Alignment
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    // Safe area
    style.DisplaySafeAreaPadding = ImVec2(3.0f * s, 3.0f * s);

    // Anti-aliasing
    style.AntiAliasedLines = true;
    style.AntiAliasedLinesUseTex = true;
    style.AntiAliasedFill = true;

    // Colors
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = tokens_.text;
    colors[ImGuiCol_TextDisabled] = tokens_.textDisabled;
    colors[ImGuiCol_WindowBg] = tokens_.windowBg;
    colors[ImGuiCol_ChildBg] = tokens_.childBg;
    colors[ImGuiCol_PopupBg] = tokens_.popupBg;
    colors[ImGuiCol_Border] = tokens_.border;
    colors[ImGuiCol_BorderShadow] = tokens_.borderShadow;
    colors[ImGuiCol_FrameBg] = tokens_.frameBg;
    colors[ImGuiCol_FrameBgHovered] = tokens_.frameBgHovered;
    colors[ImGuiCol_FrameBgActive] = tokens_.frameBgActive;
    colors[ImGuiCol_TitleBg] = tokens_.titleBg;
    colors[ImGuiCol_TitleBgActive] = tokens_.titleBgActive;
    colors[ImGuiCol_TitleBgCollapsed] = tokens_.titleBgCollapsed;
    colors[ImGuiCol_MenuBarBg] = tokens_.menuBarBg;
    colors[ImGuiCol_ScrollbarBg] = tokens_.scrollbarBg;
    colors[ImGuiCol_ScrollbarGrab] = tokens_.scrollbarGrab;
    colors[ImGuiCol_ScrollbarGrabHovered] = tokens_.scrollbarGrabHovered;
    colors[ImGuiCol_ScrollbarGrabActive] = tokens_.scrollbarGrabActive;
    colors[ImGuiCol_CheckMark] = tokens_.checkMark;
    colors[ImGuiCol_SliderGrab] = tokens_.sliderGrab;
    colors[ImGuiCol_SliderGrabActive] = tokens_.sliderGrabActive;
    colors[ImGuiCol_Button] = tokens_.button;
    colors[ImGuiCol_ButtonHovered] = tokens_.buttonHovered;
    colors[ImGuiCol_ButtonActive] = tokens_.buttonActive;
    colors[ImGuiCol_Header] = tokens_.header;
    colors[ImGuiCol_HeaderHovered] = tokens_.headerHovered;
    colors[ImGuiCol_HeaderActive] = tokens_.headerActive;
    colors[ImGuiCol_Separator] = tokens_.separator;
    colors[ImGuiCol_SeparatorHovered] = tokens_.separatorHovered;
    colors[ImGuiCol_SeparatorActive] = tokens_.separatorActive;
    colors[ImGuiCol_ResizeGrip] = tokens_.resizeGrip;
    colors[ImGuiCol_ResizeGripHovered] = tokens_.resizeGripHovered;
    colors[ImGuiCol_ResizeGripActive] = tokens_.resizeGripActive;
    colors[ImGuiCol_Tab] = tokens_.tab;
    colors[ImGuiCol_TabHovered] = tokens_.tabHovered;
    colors[ImGuiCol_TabActive] = tokens_.tabActive;
    colors[ImGuiCol_TabUnfocused] = tokens_.tabUnfocused;
    colors[ImGuiCol_TabUnfocusedActive] = tokens_.tabUnfocusedActive;
    colors[ImGuiCol_DockingPreview] = tokens_.dockingPreview;
    colors[ImGuiCol_DockingEmptyBg] = tokens_.dockingEmptyBg;
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.0f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.43f, 0.35f, 1.0f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.0f, 1.0f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.60f, 0.0f, 1.0f);
    colors[ImGuiCol_TableHeaderBg] = tokens_.tableHeaderBg;
    colors[ImGuiCol_TableBorderStrong] = tokens_.tableBorderStrong;
    colors[ImGuiCol_TableBorderLight] = tokens_.tableBorderLight;
    colors[ImGuiCol_TableRowBg] = tokens_.tableRowBg;
    colors[ImGuiCol_TableRowBgAlt] = tokens_.tableRowBgAlt;
    colors[ImGuiCol_TextSelectedBg] = tokens_.textSelectedBg;
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
    colors[ImGuiCol_NavHighlight] = tokens_.navHighlight;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.35f);
}

// Simple JSON parsing helpers (no external dependencies)
namespace
{
    std::string trim(const std::string& str)
    {
        auto start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        auto end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    bool parseJsonValue(const std::string& json, const std::string& key, std::string& value)
    {
        std::string searchKey = "\"" + key + "\"";
        auto pos = json.find(searchKey);
        if (pos == std::string::npos) return false;

        pos = json.find(':', pos);
        if (pos == std::string::npos) return false;

        // Skip whitespace
        pos++;
        while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) pos++;

        // Handle string values
        if (json[pos] == '"')
        {
            auto endPos = json.find('"', pos + 1);
            if (endPos != std::string::npos)
            {
                value = json.substr(pos + 1, endPos - pos - 1);
                return true;
            }
        }
        // Handle numeric values
        else
        {
            auto endPos = json.find_first_of(",}\n", pos);
            if (endPos != std::string::npos)
            {
                value = trim(json.substr(pos, endPos - pos));
                return true;
            }
        }
        return false;
    }

    bool parseJsonFloat(const std::string& json, const std::string& key, float& value)
    {
        std::string strValue;
        if (parseJsonValue(json, key, strValue))
        {
            try
            {
                value = std::stof(strValue);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }
        return false;
    }

    ImVec4 parseColorFromJson(const std::string& json, const std::string& key, const ImVec4& defaultColor)
    {
        std::string value;
        if (!parseJsonValue(json, key, value)) return defaultColor;

        // Parse hex color #RRGGBB or #RRGGBBAA
        if (!value.empty() && value[0] == '#')
        {
            unsigned int r = 0, g = 0, b = 0, a = 255;
            if (value.size() == 7) // #RRGGBB
            {
                if (std::sscanf(value.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3)
                {
                    return ImVec4(
                        static_cast<float>(r) / 255.0f,
                        static_cast<float>(g) / 255.0f,
                        static_cast<float>(b) / 255.0f,
                        1.0f
                    );
                }
            }
            else if (value.size() == 9) // #RRGGBBAA
            {
                if (std::sscanf(value.c_str(), "#%02x%02x%02x%02x", &r, &g, &b, &a) == 4)
                {
                    return ImVec4(
                        static_cast<float>(r) / 255.0f,
                        static_cast<float>(g) / 255.0f,
                        static_cast<float>(b) / 255.0f,
                        static_cast<float>(a) / 255.0f
                    );
                }
            }
        }

        return defaultColor;
    }
}

bool Theme::loadFromFile(const std::filesystem::path& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    // Reset to defaults first
    applyDefaultTokens();

    // Parse colors
    tokens_.windowBg = parseColorFromJson(json, "windowBg", tokens_.windowBg);
    tokens_.childBg = parseColorFromJson(json, "childBg", tokens_.childBg);
    tokens_.popupBg = parseColorFromJson(json, "popupBg", tokens_.popupBg);
    tokens_.border = parseColorFromJson(json, "border", tokens_.border);
    tokens_.titleBg = parseColorFromJson(json, "titleBg", tokens_.titleBg);
    tokens_.titleBgActive = parseColorFromJson(json, "titleBgActive", tokens_.titleBgActive);
    tokens_.menuBarBg = parseColorFromJson(json, "menuBarBg", tokens_.menuBarBg);
    tokens_.button = parseColorFromJson(json, "button", tokens_.button);
    tokens_.buttonHovered = parseColorFromJson(json, "buttonHovered", tokens_.buttonHovered);
    tokens_.buttonActive = parseColorFromJson(json, "buttonActive", tokens_.buttonActive);
    tokens_.frameBg = parseColorFromJson(json, "frameBg", tokens_.frameBg);
    tokens_.frameBgHovered = parseColorFromJson(json, "frameBgHovered", tokens_.frameBgHovered);
    tokens_.frameBgActive = parseColorFromJson(json, "frameBgActive", tokens_.frameBgActive);
    tokens_.text = parseColorFromJson(json, "text", tokens_.text);
    tokens_.textDisabled = parseColorFromJson(json, "textDisabled", tokens_.textDisabled);

    // DAW-specific colors
    tokens_.meterGreen = parseColorFromJson(json, "meterGreen", tokens_.meterGreen);
    tokens_.meterYellow = parseColorFromJson(json, "meterYellow", tokens_.meterYellow);
    tokens_.meterRed = parseColorFromJson(json, "meterRed", tokens_.meterRed);
    tokens_.meterBackground = parseColorFromJson(json, "meterBackground", tokens_.meterBackground);
    tokens_.gridLine = parseColorFromJson(json, "gridLine", tokens_.gridLine);
    tokens_.gridLineBeat = parseColorFromJson(json, "gridLineBeat", tokens_.gridLineBeat);
    tokens_.gridLineBar = parseColorFromJson(json, "gridLineBar", tokens_.gridLineBar);
    tokens_.playhead = parseColorFromJson(json, "playhead", tokens_.playhead);
    tokens_.selection = parseColorFromJson(json, "selection", tokens_.selection);
    tokens_.noteOn = parseColorFromJson(json, "noteOn", tokens_.noteOn);
    tokens_.noteOff = parseColorFromJson(json, "noteOff", tokens_.noteOff);
    tokens_.playButton = parseColorFromJson(json, "playButton", tokens_.playButton);
    tokens_.stopButton = parseColorFromJson(json, "stopButton", tokens_.stopButton);
    tokens_.recordButton = parseColorFromJson(json, "recordButton", tokens_.recordButton);

    // Parse layout values
    parseJsonFloat(json, "spacingXs", tokens_.spacingXs);
    parseJsonFloat(json, "spacingSm", tokens_.spacingSm);
    parseJsonFloat(json, "spacingMd", tokens_.spacingMd);
    parseJsonFloat(json, "spacingLg", tokens_.spacingLg);
    parseJsonFloat(json, "spacingXl", tokens_.spacingXl);
    parseJsonFloat(json, "radiusSm", tokens_.radiusSm);
    parseJsonFloat(json, "radiusMd", tokens_.radiusMd);
    parseJsonFloat(json, "radiusLg", tokens_.radiusLg);
    parseJsonFloat(json, "borderWidth", tokens_.borderWidth);
    parseJsonFloat(json, "scrollbarSize", tokens_.scrollbarSize);

    // Parse typography
    parseJsonFloat(json, "fontSizeXs", tokens_.fontSizeXs);
    parseJsonFloat(json, "fontSizeSm", tokens_.fontSizeSm);
    parseJsonFloat(json, "fontSizeMd", tokens_.fontSizeMd);
    parseJsonFloat(json, "fontSizeLg", tokens_.fontSizeLg);
    parseJsonFloat(json, "fontSizeXl", tokens_.fontSizeXl);

    // Parse animation timing
    parseJsonFloat(json, "animDurationFast", tokens_.animDurationFast);
    parseJsonFloat(json, "animDurationNormal", tokens_.animDurationNormal);
    parseJsonFloat(json, "animDurationSlow", tokens_.animDurationSlow);

    // Store path and modification time
    currentPath_ = filepath;
    std::error_code ec;
    lastModified_ = std::filesystem::last_write_time(filepath, ec);

    return true;
}

bool Theme::saveToFile(const std::filesystem::path& filepath) const
{
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    auto colorToHex = [](const ImVec4& c) -> std::string
    {
        char buf[16];
        if (c.w >= 0.999f)
        {
            std::snprintf(buf, sizeof(buf), "#%02X%02X%02X",
                static_cast<int>(c.x * 255.0f),
                static_cast<int>(c.y * 255.0f),
                static_cast<int>(c.z * 255.0f));
        }
        else
        {
            std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X",
                static_cast<int>(c.x * 255.0f),
                static_cast<int>(c.y * 255.0f),
                static_cast<int>(c.z * 255.0f),
                static_cast<int>(c.w * 255.0f));
        }
        return buf;
    };

    file << "{\n";
    file << "  \"name\": \"Default Theme\",\n";
    file << "  \"version\": \"1.0.0\",\n";
    file << "\n";
    file << "  \"_comment_colors\": \"UI Colors in #RRGGBB or #RRGGBBAA format\",\n";
    file << "\n";
    file << "  \"windowBg\": \"" << colorToHex(tokens_.windowBg) << "\",\n";
    file << "  \"childBg\": \"" << colorToHex(tokens_.childBg) << "\",\n";
    file << "  \"popupBg\": \"" << colorToHex(tokens_.popupBg) << "\",\n";
    file << "  \"border\": \"" << colorToHex(tokens_.border) << "\",\n";
    file << "  \"titleBg\": \"" << colorToHex(tokens_.titleBg) << "\",\n";
    file << "  \"titleBgActive\": \"" << colorToHex(tokens_.titleBgActive) << "\",\n";
    file << "  \"menuBarBg\": \"" << colorToHex(tokens_.menuBarBg) << "\",\n";
    file << "  \"button\": \"" << colorToHex(tokens_.button) << "\",\n";
    file << "  \"buttonHovered\": \"" << colorToHex(tokens_.buttonHovered) << "\",\n";
    file << "  \"buttonActive\": \"" << colorToHex(tokens_.buttonActive) << "\",\n";
    file << "  \"frameBg\": \"" << colorToHex(tokens_.frameBg) << "\",\n";
    file << "  \"frameBgHovered\": \"" << colorToHex(tokens_.frameBgHovered) << "\",\n";
    file << "  \"frameBgActive\": \"" << colorToHex(tokens_.frameBgActive) << "\",\n";
    file << "  \"text\": \"" << colorToHex(tokens_.text) << "\",\n";
    file << "  \"textDisabled\": \"" << colorToHex(tokens_.textDisabled) << "\",\n";
    file << "\n";
    file << "  \"_comment_daw_colors\": \"DAW-specific colors\",\n";
    file << "\n";
    file << "  \"meterGreen\": \"" << colorToHex(tokens_.meterGreen) << "\",\n";
    file << "  \"meterYellow\": \"" << colorToHex(tokens_.meterYellow) << "\",\n";
    file << "  \"meterRed\": \"" << colorToHex(tokens_.meterRed) << "\",\n";
    file << "  \"meterBackground\": \"" << colorToHex(tokens_.meterBackground) << "\",\n";
    file << "  \"gridLine\": \"" << colorToHex(tokens_.gridLine) << "\",\n";
    file << "  \"gridLineBeat\": \"" << colorToHex(tokens_.gridLineBeat) << "\",\n";
    file << "  \"gridLineBar\": \"" << colorToHex(tokens_.gridLineBar) << "\",\n";
    file << "  \"playhead\": \"" << colorToHex(tokens_.playhead) << "\",\n";
    file << "  \"selection\": \"" << colorToHex(tokens_.selection) << "\",\n";
    file << "  \"noteOn\": \"" << colorToHex(tokens_.noteOn) << "\",\n";
    file << "  \"noteOff\": \"" << colorToHex(tokens_.noteOff) << "\",\n";
    file << "  \"playButton\": \"" << colorToHex(tokens_.playButton) << "\",\n";
    file << "  \"stopButton\": \"" << colorToHex(tokens_.stopButton) << "\",\n";
    file << "  \"recordButton\": \"" << colorToHex(tokens_.recordButton) << "\",\n";
    file << "\n";
    file << "  \"_comment_layout\": \"Layout spacing in pixels (8px grid)\",\n";
    file << "\n";
    file << "  \"spacingXs\": " << tokens_.spacingXs << ",\n";
    file << "  \"spacingSm\": " << tokens_.spacingSm << ",\n";
    file << "  \"spacingMd\": " << tokens_.spacingMd << ",\n";
    file << "  \"spacingLg\": " << tokens_.spacingLg << ",\n";
    file << "  \"spacingXl\": " << tokens_.spacingXl << ",\n";
    file << "  \"radiusSm\": " << tokens_.radiusSm << ",\n";
    file << "  \"radiusMd\": " << tokens_.radiusMd << ",\n";
    file << "  \"radiusLg\": " << tokens_.radiusLg << ",\n";
    file << "  \"borderWidth\": " << tokens_.borderWidth << ",\n";
    file << "  \"scrollbarSize\": " << tokens_.scrollbarSize << ",\n";
    file << "\n";
    file << "  \"_comment_typography\": \"Font sizes in points\",\n";
    file << "\n";
    file << "  \"fontSizeXs\": " << tokens_.fontSizeXs << ",\n";
    file << "  \"fontSizeSm\": " << tokens_.fontSizeSm << ",\n";
    file << "  \"fontSizeMd\": " << tokens_.fontSizeMd << ",\n";
    file << "  \"fontSizeLg\": " << tokens_.fontSizeLg << ",\n";
    file << "  \"fontSizeXl\": " << tokens_.fontSizeXl << ",\n";
    file << "\n";
    file << "  \"_comment_animation\": \"Animation timing in seconds\",\n";
    file << "\n";
    file << "  \"animDurationFast\": " << tokens_.animDurationFast << ",\n";
    file << "  \"animDurationNormal\": " << tokens_.animDurationNormal << ",\n";
    file << "  \"animDurationSlow\": " << tokens_.animDurationSlow << "\n";
    file << "}\n";

    return true;
}

bool Theme::checkFileModified() const
{
    if (currentPath_.empty()) return false;

    std::error_code ec;
    auto currentTime = std::filesystem::last_write_time(currentPath_, ec);
    if (ec) return false;

    return currentTime != lastModified_;
}

bool Theme::reloadIfModified()
{
    if (checkFileModified())
    {
        return loadFromFile(currentPath_);
    }
    return false;
}

ImVec4 Theme::parseColor(const std::string& json)
{
    return parseColorFromJson(json, "", ImVec4(1, 1, 1, 1));
}

std::string Theme::colorToJson(const ImVec4& color)
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X",
        static_cast<int>(color.x * 255),
        static_cast<int>(color.y * 255),
        static_cast<int>(color.z * 255),
        static_cast<int>(color.w * 255));
    return buf;
}

} // namespace daw::ui::imgui
