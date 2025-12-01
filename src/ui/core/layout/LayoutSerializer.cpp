/**
 * @file LayoutSerializer.cpp
 * @brief Layout persistence implementation
 */

#include "LayoutSerializer.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace daw::ui::layout
{

// Simple JSON helpers (avoiding external dependencies)
namespace json
{

std::string escape(const std::string& s)
{
    std::string result;
    result.reserve(s.size() + 10);
    for (char c : s) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string unescape(const std::string& s)
{
    std::string result;
    result.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            switch (s[i + 1]) {
                case '"': result += '"'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case 'n': result += '\n'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case 't': result += '\t'; ++i; break;
                default: result += s[i]; break;
            }
        } else {
            result += s[i];
        }
    }
    return result;
}

// Extract string value from JSON
std::string extractString(const std::string& json, const std::string& key)
{
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    
    auto endPos = pos + 1;
    while (endPos < json.size() && !(json[endPos] == '"' && json[endPos - 1] != '\\')) {
        ++endPos;
    }
    
    return unescape(json.substr(pos + 1, endPos - pos - 1));
}

// Extract number value from JSON
double extractNumber(const std::string& json, const std::string& key, double defaultVal = 0.0)
{
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find(searchKey);
    if (pos == std::string::npos) return defaultVal;
    
    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultVal;
    
    // Skip whitespace
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) {
        ++pos;
    }
    
    // Find end of number
    auto endPos = pos;
    while (endPos < json.size() && (std::isdigit(json[endPos]) || json[endPos] == '.' ||
                                     json[endPos] == '-' || json[endPos] == '+' ||
                                     json[endPos] == 'e' || json[endPos] == 'E')) {
        ++endPos;
    }
    
    try {
        return std::stod(json.substr(pos, endPos - pos));
    } catch (...) {
        return defaultVal;
    }
}

// Extract boolean value from JSON
bool extractBool(const std::string& json, const std::string& key, bool defaultVal = false)
{
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find(searchKey);
    if (pos == std::string::npos) return defaultVal;
    
    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultVal;
    
    auto truePos = json.find("true", pos);
    auto falsePos = json.find("false", pos);
    auto nextComma = json.find(',', pos);
    auto nextBrace = json.find('}', pos);
    
    std::size_t endPos = std::min(nextComma, nextBrace);
    
    if (truePos != std::string::npos && truePos < endPos) return true;
    if (falsePos != std::string::npos && falsePos < endPos) return false;
    
    return defaultVal;
}

} // namespace json

// ============================================================================
// LayoutSerializer Implementation
// ============================================================================

LayoutSerializer::LayoutSerializer() = default;
LayoutSerializer::~LayoutSerializer() = default;

std::optional<LayoutState> LayoutSerializer::load(const std::filesystem::path& filepath)
{
    if (!std::filesystem::exists(filepath)) {
        return std::nullopt;
    }
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return std::nullopt;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    auto state = deserializeFromJson(content);
    if (!state) {
        return std::nullopt;
    }
    
    // Migrate if needed
    if (!migrateIfNeeded(*state)) {
        return std::nullopt;
    }
    
    // Validate
    if (!validate(*state)) {
        return std::nullopt;
    }
    
    return state;
}

bool LayoutSerializer::save(const LayoutState& state, const std::filesystem::path& filepath)
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
    
    file << serializeToJson(state);
    return file.good();
}

void LayoutSerializer::registerMigration(int fromVersion, MigrationFunc migration)
{
    migrations_[fromVersion] = std::move(migration);
}

void LayoutSerializer::enableAutosave(const std::filesystem::path& filepath, int debounceMs)
{
    autosaveEnabled_ = true;
    autosavePath_ = filepath;
    autosaveDebounceMs_ = debounceMs;
}

void LayoutSerializer::disableAutosave()
{
    autosaveEnabled_ = false;
}

void LayoutSerializer::markChanged()
{
    lastChangeTime_ = std::chrono::steady_clock::now();
    pendingAutosave_ = true;
}

void LayoutSerializer::update(const LayoutState& state)
{
    if (!autosaveEnabled_ || !pendingAutosave_) return;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastChangeTime_).count();
    
    if (elapsed >= autosaveDebounceMs_) {
        save(state, autosavePath_);
        pendingAutosave_ = false;
    }
}

void LayoutSerializer::saveNow(const LayoutState& state)
{
    if (autosaveEnabled_) {
        save(state, autosavePath_);
        pendingAutosave_ = false;
    }
}

LayoutState LayoutSerializer::getDefaultLayout()
{
    LayoutState state;
    state.version = LayoutState::CURRENT_VERSION;
    state.themePath = "assets/themes/default.json";
    state.fontScale = 1.0f;
    state.dpiScale = 1.0f;
    state.windowWidth = 1920;
    state.windowHeight = 1080;
    state.windowMaximized = false;
    
    // Default panel layout
    state.panels = {
        {"transport", true, false, 0, 0, 0, 40, -1, "top"},
        {"browser", true, false, 0, 40, 250, 0, -1, "left"},
        {"channel_rack", true, false, 250, 40, 0, 300, -1, "center"},
        {"piano_roll", true, false, 250, 340, 0, 0, -1, "center"},
        {"playlist", true, false, 250, 40, 0, 0, -1, "center"},
        {"mixer", true, false, 0, 0, 0, 200, -1, "bottom"},
        {"inspector", true, false, 0, 40, 250, 0, -1, "right"}
    };
    
    return state;
}

bool LayoutSerializer::validate(const LayoutState& state)
{
    // Version check
    if (state.version < 1 || state.version > LayoutState::CURRENT_VERSION) {
        return false;
    }
    
    // Validate font scale
    if (state.fontScale < 0.5f || state.fontScale > 4.0f) {
        return false;
    }
    
    // Validate DPI scale
    if (state.dpiScale < 0.5f || state.dpiScale > 4.0f) {
        return false;
    }
    
    // Validate window size
    if (state.windowWidth < 800 || state.windowHeight < 600) {
        return false;
    }
    
    return true;
}

std::string LayoutSerializer::toIni(const LayoutState& state)
{
    // Convert panel layout to ImGui INI format
    std::ostringstream ss;
    
    for (const auto& panel : state.panels) {
        ss << "[Window][" << panel.id << "]\n";
        ss << "Pos=" << static_cast<int>(panel.posX) << ","
           << static_cast<int>(panel.posY) << "\n";
        if (panel.width > 0 && panel.height > 0) {
            ss << "Size=" << static_cast<int>(panel.width) << ","
               << static_cast<int>(panel.height) << "\n";
        }
        ss << "Collapsed=" << (panel.collapsed ? "1" : "0") << "\n";
        if (panel.dockId >= 0) {
            ss << "DockId=0x" << std::hex << panel.dockId << std::dec << "\n";
        }
        ss << "\n";
    }
    
    return ss.str();
}

void LayoutSerializer::fromIni(LayoutState& state, const std::string& ini)
{
    state.dockLayoutIni = ini;
}

bool LayoutSerializer::migrateIfNeeded(LayoutState& state)
{
    while (state.version < LayoutState::CURRENT_VERSION) {
        auto it = migrations_.find(state.version);
        if (it != migrations_.end()) {
            if (!it->second(state, state.version)) {
                return false;
            }
            state.version++;
        } else {
            // No migration available, just bump version
            state.version++;
        }
    }
    return true;
}

std::string LayoutSerializer::serializeToJson(const LayoutState& state) const
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    ss << "{\n";
    ss << "  \"version\": " << state.version << ",\n";
    ss << "  \"themePath\": \"" << json::escape(state.themePath) << "\",\n";
    ss << "  \"fontScale\": " << state.fontScale << ",\n";
    ss << "  \"dpiScale\": " << state.dpiScale << ",\n";
    ss << "  \"windowWidth\": " << state.windowWidth << ",\n";
    ss << "  \"windowHeight\": " << state.windowHeight << ",\n";
    ss << "  \"windowMaximized\": " << (state.windowMaximized ? "true" : "false") << ",\n";
    
    // Panels
    ss << "  \"panels\": [\n";
    for (std::size_t i = 0; i < state.panels.size(); ++i) {
        const auto& panel = state.panels[i];
        ss << "    {\n";
        ss << "      \"id\": \"" << json::escape(panel.id) << "\",\n";
        ss << "      \"visible\": " << (panel.visible ? "true" : "false") << ",\n";
        ss << "      \"collapsed\": " << (panel.collapsed ? "true" : "false") << ",\n";
        ss << "      \"posX\": " << panel.posX << ",\n";
        ss << "      \"posY\": " << panel.posY << ",\n";
        ss << "      \"width\": " << panel.width << ",\n";
        ss << "      \"height\": " << panel.height << ",\n";
        ss << "      \"dockId\": " << panel.dockId << ",\n";
        ss << "      \"dockPosition\": \"" << json::escape(panel.dockPosition) << "\"\n";
        ss << "    }" << (i + 1 < state.panels.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";
    
    // Dock layout INI
    ss << "  \"dockLayoutIni\": \"" << json::escape(state.dockLayoutIni) << "\",\n";
    
    // Custom settings
    ss << "  \"customSettings\": {\n";
    std::size_t customIdx = 0;
    for (const auto& [key, value] : state.customSettings) {
        ss << "    \"" << json::escape(key) << "\": \"" << json::escape(value) << "\"";
        ss << (++customIdx < state.customSettings.size() ? "," : "") << "\n";
    }
    ss << "  }\n";
    
    ss << "}\n";
    
    return ss.str();
}

std::optional<LayoutState> LayoutSerializer::deserializeFromJson(const std::string& jsonContent)
{
    LayoutState state;
    
    // Extract basic fields
    state.version = static_cast<int>(json::extractNumber(jsonContent, "version", 1));
    state.themePath = json::extractString(jsonContent, "themePath");
    state.fontScale = static_cast<float>(json::extractNumber(jsonContent, "fontScale", 1.0));
    state.dpiScale = static_cast<float>(json::extractNumber(jsonContent, "dpiScale", 1.0));
    state.windowWidth = static_cast<int>(json::extractNumber(jsonContent, "windowWidth", 1920));
    state.windowHeight = static_cast<int>(json::extractNumber(jsonContent, "windowHeight", 1080));
    state.windowMaximized = json::extractBool(jsonContent, "windowMaximized", false);
    state.dockLayoutIni = json::extractString(jsonContent, "dockLayoutIni");
    
    // Parse panels array (simplified - real implementation would use proper JSON parser)
    auto panelsStart = jsonContent.find("\"panels\"");
    if (panelsStart != std::string::npos) {
        auto arrayStart = jsonContent.find('[', panelsStart);
        auto arrayEnd = jsonContent.find(']', arrayStart);
        if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
            std::string panelsJson = jsonContent.substr(arrayStart, arrayEnd - arrayStart + 1);
            
            // Find each panel object
            std::size_t objStart = 0;
            while ((objStart = panelsJson.find('{', objStart)) != std::string::npos) {
                auto objEnd = panelsJson.find('}', objStart);
                if (objEnd == std::string::npos) break;
                
                std::string panelJson = panelsJson.substr(objStart, objEnd - objStart + 1);
                
                PanelState panel;
                panel.id = json::extractString(panelJson, "id");
                panel.visible = json::extractBool(panelJson, "visible", true);
                panel.collapsed = json::extractBool(panelJson, "collapsed", false);
                panel.posX = static_cast<float>(json::extractNumber(panelJson, "posX", 0));
                panel.posY = static_cast<float>(json::extractNumber(panelJson, "posY", 0));
                panel.width = static_cast<float>(json::extractNumber(panelJson, "width", 0));
                panel.height = static_cast<float>(json::extractNumber(panelJson, "height", 0));
                panel.dockId = static_cast<int>(json::extractNumber(panelJson, "dockId", -1));
                panel.dockPosition = json::extractString(panelJson, "dockPosition");
                
                if (!panel.id.empty()) {
                    state.panels.push_back(panel);
                }
                
                objStart = objEnd + 1;
            }
        }
    }
    
    return state;
}

} // namespace daw::ui::layout
