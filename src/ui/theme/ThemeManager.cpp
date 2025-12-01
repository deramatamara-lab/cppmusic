/**
 * @file ThemeManager.cpp
 * @brief Enhanced theme manager implementation
 */

#include "ThemeManager.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace daw::ui::theme
{

// ============================================================================
// Color Implementation
// ============================================================================

Color Color::fromHex(const std::string& hex)
{
    Color c;
    std::string h = hex;
    
    // Remove # prefix
    if (!h.empty() && h[0] == '#') {
        h = h.substr(1);
    }
    
    if (h.length() == 6 || h.length() == 8) {
        unsigned int value = 0;
        std::stringstream ss;
        ss << std::hex << h;
        ss >> value;
        
        if (h.length() == 8) {
            c.r = static_cast<float>((value >> 24) & 0xFF) / 255.0f;
            c.g = static_cast<float>((value >> 16) & 0xFF) / 255.0f;
            c.b = static_cast<float>((value >> 8) & 0xFF) / 255.0f;
            c.a = static_cast<float>(value & 0xFF) / 255.0f;
        } else {
            c.r = static_cast<float>((value >> 16) & 0xFF) / 255.0f;
            c.g = static_cast<float>((value >> 8) & 0xFF) / 255.0f;
            c.b = static_cast<float>(value & 0xFF) / 255.0f;
            c.a = 1.0f;
        }
    }
    
    return c;
}

std::string Color::toHex() const
{
    std::ostringstream ss;
    ss << "#"
       << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(r * 255)
       << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(g * 255)
       << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b * 255);
    if (a < 0.99f) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(a * 255);
    }
    return ss.str();
}

Color Color::hover(float amount) const
{
    return Color(
        std::min(1.0f, r + amount),
        std::min(1.0f, g + amount),
        std::min(1.0f, b + amount),
        a
    );
}

Color Color::disabled() const
{
    // Desaturate and reduce opacity
    float gray = 0.299f * r + 0.587f * g + 0.114f * b;
    return Color(
        r * 0.5f + gray * 0.5f,
        g * 0.5f + gray * 0.5f,
        b * 0.5f + gray * 0.5f,
        a * 0.6f
    );
}

Color Color::pressed(float amount) const
{
    return Color(
        std::max(0.0f, r - amount),
        std::max(0.0f, g - amount),
        std::max(0.0f, b - amount),
        a
    );
}

Color Color::mix(const Color& other, float t) const
{
    return Color(
        r + (other.r - r) * t,
        g + (other.g - g) * t,
        b + (other.b - b) * t,
        a + (other.a - a) * t
    );
}

Color Color::withAlpha(float alpha) const
{
    return Color(r, g, b, alpha);
}

float* Color::toImVec4(float* out) const
{
    out[0] = r;
    out[1] = g;
    out[2] = b;
    out[3] = a;
    return out;
}

// ============================================================================
// Token accessors
// ============================================================================

float Spacing::get(const std::string& size) const
{
    if (size == "xs") return xs;
    if (size == "sm") return sm;
    if (size == "md") return md;
    if (size == "lg") return lg;
    if (size == "xl") return xl;
    if (size == "xxl") return xxl;
    return md;
}

float Radii::get(const std::string& size) const
{
    if (size == "none") return none;
    if (size == "sm") return sm;
    if (size == "md") return md;
    if (size == "lg") return lg;
    if (size == "xl") return xl;
    if (size == "full") return full;
    return md;
}

float Typography::getFontSize(const std::string& size) const
{
    if (size == "xs") return fontSizeXs;
    if (size == "sm") return fontSizeSm;
    if (size == "md") return fontSizeMd;
    if (size == "lg") return fontSizeLg;
    if (size == "xl") return fontSizeXl;
    if (size == "xxl") return fontSizeXxl;
    return fontSizeMd;
}

const Shadow& Elevations::get(const std::string& level) const
{
    if (level == "none") return none;
    if (level == "sm") return sm;
    if (level == "md") return md;
    if (level == "lg") return lg;
    if (level == "xl") return xl;
    return md;
}

// ============================================================================
// ThemeManager Implementation
// ============================================================================

ThemeManager::ThemeManager()
{
    currentTokens_ = getDefaultTheme();
    currentThemeName_ = "Default";
    themes_["Default"] = currentTokens_;
    themes_["High Contrast"] = getHighContrastTheme();
}

bool ThemeManager::loadFromFile(const std::filesystem::path& filepath)
{
    if (!std::filesystem::exists(filepath)) {
        return false;
    }
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    ThemeTokens tokens;
    if (!parseThemeJson(content, tokens)) {
        return false;
    }
    
    if (tokens.name.empty()) {
        tokens.name = filepath.stem().string();
    }
    
    themes_[tokens.name] = tokens;
    currentPath_ = filepath;
    lastModified_ = std::filesystem::last_write_time(filepath);
    
    return setTheme(tokens.name);
}

bool ThemeManager::saveToFile(const std::filesystem::path& filepath) const
{
    // Create parent directories if needed
    auto parent = filepath.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
        std::filesystem::create_directories(parent);
    }
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    file << serializeTheme(currentTokens_);
    return file.good();
}

int ThemeManager::loadAllThemes(const std::filesystem::path& directory)
{
    if (!std::filesystem::exists(directory)) {
        return 0;
    }
    
    int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            if (loadFromFile(entry.path())) {
                count++;
            }
        }
    }
    
    return count;
}

std::vector<std::string> ThemeManager::getAvailableThemes() const
{
    std::vector<std::string> names;
    names.reserve(themes_.size());
    
    for (const auto& [name, tokens] : themes_) {
        names.push_back(name);
    }
    
    std::sort(names.begin(), names.end());
    return names;
}

bool ThemeManager::setTheme(const std::string& name)
{
    auto it = themes_.find(name);
    if (it == themes_.end()) {
        return false;
    }
    
    currentTokens_ = it->second;
    currentThemeName_ = name;
    applyToImGui();
    notifyListeners();
    
    return true;
}

void ThemeManager::applyToImGui()
{
    ImGuiStyle& style = ImGui::GetStyle();
    const auto& c = currentTokens_.colors;
    const auto& s = currentTokens_.spacing;
    const auto& r = currentTokens_.radii;
    
    // Apply DPI and font scaling
    float scale = currentTokens_.dpiScale * currentTokens_.fontScale;
    
    // Spacing
    style.WindowPadding = ImVec2(s.md * scale, s.md * scale);
    style.FramePadding = ImVec2(s.sm * scale, s.xs * scale);
    style.ItemSpacing = ImVec2(s.sm * scale, s.xs * scale);
    style.ItemInnerSpacing = ImVec2(s.xs * scale, s.xs * scale);
    style.IndentSpacing = s.lg * scale;
    style.ScrollbarSize = 12.0f * scale;
    style.GrabMinSize = 10.0f * scale;
    
    // Borders and rounding
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
    
    style.WindowRounding = r.md * scale;
    style.ChildRounding = r.sm * scale;
    style.FrameRounding = r.sm * scale;
    style.PopupRounding = r.md * scale;
    style.ScrollbarRounding = r.lg * scale;
    style.GrabRounding = r.sm * scale;
    style.TabRounding = r.sm * scale;
    
    // Colors
    ImVec4* colors = style.Colors;
    
    colors[ImGuiCol_Text] = ImVec4(c.textPrimary.r, c.textPrimary.g, c.textPrimary.b, c.textPrimary.a);
    colors[ImGuiCol_TextDisabled] = ImVec4(c.textMuted.r, c.textMuted.g, c.textMuted.b, c.textMuted.a);
    colors[ImGuiCol_WindowBg] = ImVec4(c.bgPrimary.r, c.bgPrimary.g, c.bgPrimary.b, c.bgPrimary.a);
    colors[ImGuiCol_ChildBg] = ImVec4(c.bgSecondary.r, c.bgSecondary.g, c.bgSecondary.b, c.bgSecondary.a);
    colors[ImGuiCol_PopupBg] = ImVec4(c.bgElevated.r, c.bgElevated.g, c.bgElevated.b, c.bgElevated.a);
    colors[ImGuiCol_Border] = ImVec4(c.borderLight.r, c.borderLight.g, c.borderLight.b, c.borderLight.a);
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    
    colors[ImGuiCol_FrameBg] = ImVec4(c.bgTertiary.r, c.bgTertiary.g, c.bgTertiary.b, c.bgTertiary.a);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(c.bgElevated.r, c.bgElevated.g, c.bgElevated.b, c.bgElevated.a);
    colors[ImGuiCol_FrameBgActive] = ImVec4(c.primary.r, c.primary.g, c.primary.b, 0.7f);
    
    colors[ImGuiCol_TitleBg] = ImVec4(c.bgPrimary.r, c.bgPrimary.g, c.bgPrimary.b, c.bgPrimary.a);
    colors[ImGuiCol_TitleBgActive] = ImVec4(c.primary.r * 0.6f, c.primary.g * 0.6f, c.primary.b * 0.6f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(c.bgPrimary.r, c.bgPrimary.g, c.bgPrimary.b, 0.8f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(c.bgSecondary.r, c.bgSecondary.g, c.bgSecondary.b, c.bgSecondary.a);
    
    colors[ImGuiCol_ScrollbarBg] = ImVec4(c.bgPrimary.r, c.bgPrimary.g, c.bgPrimary.b, 0.6f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(c.borderMedium.r, c.borderMedium.g, c.borderMedium.b, c.borderMedium.a);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(c.borderMedium.r + 0.1f, c.borderMedium.g + 0.1f, c.borderMedium.b + 0.1f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(c.primary.r, c.primary.g, c.primary.b, c.primary.a);
    
    colors[ImGuiCol_CheckMark] = ImVec4(c.primary.r, c.primary.g, c.primary.b, c.primary.a);
    colors[ImGuiCol_SliderGrab] = ImVec4(c.primary.r, c.primary.g, c.primary.b, 0.8f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(c.primary.r, c.primary.g, c.primary.b, c.primary.a);
    
    colors[ImGuiCol_Button] = ImVec4(c.bgTertiary.r, c.bgTertiary.g, c.bgTertiary.b, c.bgTertiary.a);
    colors[ImGuiCol_ButtonHovered] = ImVec4(c.primary.r, c.primary.g, c.primary.b, 0.8f);
    colors[ImGuiCol_ButtonActive] = ImVec4(c.primary.r * 0.8f, c.primary.g * 0.8f, c.primary.b * 0.8f, 1.0f);
    
    colors[ImGuiCol_Header] = ImVec4(c.primary.r, c.primary.g, c.primary.b, 0.6f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(c.primary.r, c.primary.g, c.primary.b, 0.8f);
    colors[ImGuiCol_HeaderActive] = ImVec4(c.primary.r, c.primary.g, c.primary.b, c.primary.a);
    
    colors[ImGuiCol_Separator] = ImVec4(c.borderLight.r, c.borderLight.g, c.borderLight.b, c.borderLight.a);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(c.primary.r, c.primary.g, c.primary.b, 0.8f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(c.primary.r, c.primary.g, c.primary.b, c.primary.a);
    
    colors[ImGuiCol_ResizeGrip] = ImVec4(c.borderLight.r, c.borderLight.g, c.borderLight.b, 0.4f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(c.primary.r, c.primary.g, c.primary.b, 0.7f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(c.primary.r, c.primary.g, c.primary.b, 0.9f);
    
    colors[ImGuiCol_Tab] = ImVec4(c.bgTertiary.r, c.bgTertiary.g, c.bgTertiary.b, c.bgTertiary.a);
    colors[ImGuiCol_TabHovered] = ImVec4(c.primary.r, c.primary.g, c.primary.b, 0.8f);
    colors[ImGuiCol_TabSelected] = ImVec4(c.primary.r * 0.7f, c.primary.g * 0.7f, c.primary.b * 0.7f, 1.0f);
    colors[ImGuiCol_TabDimmed] = ImVec4(c.bgSecondary.r, c.bgSecondary.g, c.bgSecondary.b, c.bgSecondary.a);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(c.primary.r * 0.5f, c.primary.g * 0.5f, c.primary.b * 0.5f, 1.0f);
    
    colors[ImGuiCol_DockingPreview] = ImVec4(c.primary.r, c.primary.g, c.primary.b, 0.7f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(c.bgPrimary.r, c.bgPrimary.g, c.bgPrimary.b, c.bgPrimary.a);
    
    colors[ImGuiCol_PlotLines] = ImVec4(c.primary.r, c.primary.g, c.primary.b, c.primary.a);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(c.accent.r, c.accent.g, c.accent.b, c.accent.a);
    colors[ImGuiCol_PlotHistogram] = ImVec4(c.primary.r, c.primary.g, c.primary.b, c.primary.a);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(c.accent.r, c.accent.g, c.accent.b, c.accent.a);
    
    colors[ImGuiCol_TextSelectedBg] = ImVec4(c.selection.r, c.selection.g, c.selection.b, c.selection.a);
    colors[ImGuiCol_DragDropTarget] = ImVec4(c.accent.r, c.accent.g, c.accent.b, 0.9f);
    colors[ImGuiCol_NavHighlight] = ImVec4(c.borderFocus.r, c.borderFocus.g, c.borderFocus.b, c.borderFocus.a);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.6f);
}

void ThemeManager::setDpiScale(float scale)
{
    currentTokens_.dpiScale = std::clamp(scale, 0.5f, 4.0f);
    applyToImGui();
    notifyListeners();
}

void ThemeManager::setFontScale(float scale)
{
    currentTokens_.fontScale = std::clamp(scale, 0.5f, 4.0f);
    applyToImGui();
    notifyListeners();
}

bool ThemeManager::checkFileModified() const
{
    if (currentPath_.empty() || !std::filesystem::exists(currentPath_)) {
        return false;
    }
    
    auto currentTime = std::filesystem::last_write_time(currentPath_);
    return currentTime > lastModified_;
}

bool ThemeManager::reloadIfModified()
{
    if (checkFileModified()) {
        return loadFromFile(currentPath_);
    }
    return false;
}

void ThemeManager::onThemeChanged(ThemeChangeCallback callback)
{
    changeCallbacks_.push_back(std::move(callback));
}

void ThemeManager::notifyListeners()
{
    for (const auto& callback : changeCallbacks_) {
        callback(currentTokens_);
    }
}

ThemeTokens ThemeManager::getDefaultTheme()
{
    ThemeTokens tokens;
    tokens.name = "Default";
    tokens.description = "Default dark theme for cppmusic DAW";
    tokens.isDark = true;
    // Default values are already set in struct
    return tokens;
}

ThemeTokens ThemeManager::getHighContrastTheme()
{
    ThemeTokens tokens = getDefaultTheme();
    tokens.name = "High Contrast";
    tokens.description = "High contrast theme for accessibility";
    
    // Increase contrast
    auto& c = tokens.colors;
    c.textPrimary = Color(1.0f, 1.0f, 1.0f, 1.0f);
    c.textSecondary = Color(0.85f, 0.85f, 0.85f, 1.0f);
    c.textMuted = Color(0.7f, 0.7f, 0.7f, 1.0f);
    
    c.bgPrimary = Color(0.0f, 0.0f, 0.0f, 1.0f);
    c.bgSecondary = Color(0.05f, 0.05f, 0.05f, 1.0f);
    c.bgTertiary = Color(0.1f, 0.1f, 0.1f, 1.0f);
    
    c.borderLight = Color(0.5f, 0.5f, 0.5f, 1.0f);
    c.borderMedium = Color(0.7f, 0.7f, 0.7f, 1.0f);
    c.borderFocus = Color(1.0f, 1.0f, 0.0f, 1.0f);
    
    c.primary = Color(0.3f, 0.6f, 1.0f, 1.0f);
    c.selection = Color(1.0f, 1.0f, 0.0f, 0.4f);
    
    return tokens;
}

std::string ThemeManager::exportDiff(const ThemeTokens& base) const
{
    // Export only values that differ from base
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"_comment\": \"Theme diff from " << base.name << "\",\n";
    
    const auto& c = currentTokens_.colors;
    const auto& bc = base.colors;
    
    // Compare colors and emit differences
    if (c.primary.toHex() != bc.primary.toHex()) {
        ss << "  \"primary\": \"" << c.primary.toHex() << "\",\n";
    }
    if (c.bgPrimary.toHex() != bc.bgPrimary.toHex()) {
        ss << "  \"bgPrimary\": \"" << c.bgPrimary.toHex() << "\",\n";
    }
    // ... more comparisons would go here
    
    ss << "  \"fontScale\": " << currentTokens_.fontScale << "\n";
    ss << "}\n";
    
    return ss.str();
}

bool ThemeManager::parseThemeJson(const std::string& json, ThemeTokens& tokens)
{
    // Simple JSON parsing (same approach as other files)
    auto extractString = [&json](const std::string& key) -> std::string {
        std::string searchKey = "\"" + key + "\"";
        auto pos = json.find(searchKey);
        if (pos == std::string::npos) return "";
        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        pos = json.find('"', pos + 1);
        if (pos == std::string::npos) return "";
        auto endPos = json.find('"', pos + 1);
        if (endPos == std::string::npos) return "";
        return json.substr(pos + 1, endPos - pos - 1);
    };
    
    auto extractNumber = [&json](const std::string& key, float def) -> float {
        std::string searchKey = "\"" + key + "\"";
        auto pos = json.find(searchKey);
        if (pos == std::string::npos) return def;
        pos = json.find(':', pos);
        if (pos == std::string::npos) return def;
        ++pos;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) ++pos;
        auto endPos = pos;
        while (endPos < json.size() && (std::isdigit(json[endPos]) || json[endPos] == '.' || json[endPos] == '-')) ++endPos;
        try { return std::stof(json.substr(pos, endPos - pos)); }
        catch (...) { return def; }
    };
    
    tokens.name = extractString("name");
    tokens.version = extractString("version");
    tokens.description = extractString("description");
    
    // Parse colors
    auto parseColor = [&extractString](const std::string& key) -> std::optional<Color> {
        std::string hex = extractString(key);
        if (hex.empty()) return std::nullopt;
        return Color::fromHex(hex);
    };
    
    if (auto c = parseColor("primary")) tokens.colors.primary = *c;
    if (auto c = parseColor("bgPrimary")) tokens.colors.bgPrimary = *c;
    if (auto c = parseColor("bgSecondary")) tokens.colors.bgSecondary = *c;
    if (auto c = parseColor("textPrimary")) tokens.colors.textPrimary = *c;
    // ... more color parsing
    
    tokens.fontScale = extractNumber("fontScale", 1.0f);
    tokens.dpiScale = extractNumber("dpiScale", 1.0f);
    
    return true;
}

std::string ThemeManager::serializeTheme(const ThemeTokens& tokens) const
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    ss << "{\n";
    ss << "  \"name\": \"" << tokens.name << "\",\n";
    ss << "  \"version\": \"" << tokens.version << "\",\n";
    ss << "  \"description\": \"" << tokens.description << "\",\n";
    ss << "  \"isDark\": " << (tokens.isDark ? "true" : "false") << ",\n\n";
    
    ss << "  \"_comment_colors\": \"Color tokens\",\n";
    ss << "  \"primary\": \"" << tokens.colors.primary.toHex() << "\",\n";
    ss << "  \"secondary\": \"" << tokens.colors.secondary.toHex() << "\",\n";
    ss << "  \"accent\": \"" << tokens.colors.accent.toHex() << "\",\n";
    ss << "  \"success\": \"" << tokens.colors.success.toHex() << "\",\n";
    ss << "  \"warning\": \"" << tokens.colors.warning.toHex() << "\",\n";
    ss << "  \"error\": \"" << tokens.colors.error.toHex() << "\",\n";
    ss << "  \"bgPrimary\": \"" << tokens.colors.bgPrimary.toHex() << "\",\n";
    ss << "  \"bgSecondary\": \"" << tokens.colors.bgSecondary.toHex() << "\",\n";
    ss << "  \"bgTertiary\": \"" << tokens.colors.bgTertiary.toHex() << "\",\n";
    ss << "  \"textPrimary\": \"" << tokens.colors.textPrimary.toHex() << "\",\n";
    ss << "  \"textSecondary\": \"" << tokens.colors.textSecondary.toHex() << "\",\n";
    ss << "  \"textMuted\": \"" << tokens.colors.textMuted.toHex() << "\",\n\n";
    
    ss << "  \"_comment_spacing\": \"Spacing tokens\",\n";
    ss << "  \"spacingXs\": " << tokens.spacing.xs << ",\n";
    ss << "  \"spacingSm\": " << tokens.spacing.sm << ",\n";
    ss << "  \"spacingMd\": " << tokens.spacing.md << ",\n";
    ss << "  \"spacingLg\": " << tokens.spacing.lg << ",\n";
    ss << "  \"spacingXl\": " << tokens.spacing.xl << ",\n\n";
    
    ss << "  \"_comment_typography\": \"Typography tokens\",\n";
    ss << "  \"fontSizeSm\": " << tokens.typography.fontSizeSm << ",\n";
    ss << "  \"fontSizeMd\": " << tokens.typography.fontSizeMd << ",\n";
    ss << "  \"fontSizeLg\": " << tokens.typography.fontSizeLg << ",\n\n";
    
    ss << "  \"fontScale\": " << tokens.fontScale << ",\n";
    ss << "  \"dpiScale\": " << tokens.dpiScale << "\n";
    ss << "}\n";
    
    return ss.str();
}

} // namespace daw::ui::theme
