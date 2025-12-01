#include "Shortcuts.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstring>

namespace daw::ui::imgui
{

// Key name mappings
static const std::unordered_map<std::string, ImGuiKey> kKeyNameMap = {
    {"A", ImGuiKey_A}, {"B", ImGuiKey_B}, {"C", ImGuiKey_C}, {"D", ImGuiKey_D},
    {"E", ImGuiKey_E}, {"F", ImGuiKey_F}, {"G", ImGuiKey_G}, {"H", ImGuiKey_H},
    {"I", ImGuiKey_I}, {"J", ImGuiKey_J}, {"K", ImGuiKey_K}, {"L", ImGuiKey_L},
    {"M", ImGuiKey_M}, {"N", ImGuiKey_N}, {"O", ImGuiKey_O}, {"P", ImGuiKey_P},
    {"Q", ImGuiKey_Q}, {"R", ImGuiKey_R}, {"S", ImGuiKey_S}, {"T", ImGuiKey_T},
    {"U", ImGuiKey_U}, {"V", ImGuiKey_V}, {"W", ImGuiKey_W}, {"X", ImGuiKey_X},
    {"Y", ImGuiKey_Y}, {"Z", ImGuiKey_Z},
    {"0", ImGuiKey_0}, {"1", ImGuiKey_1}, {"2", ImGuiKey_2}, {"3", ImGuiKey_3},
    {"4", ImGuiKey_4}, {"5", ImGuiKey_5}, {"6", ImGuiKey_6}, {"7", ImGuiKey_7},
    {"8", ImGuiKey_8}, {"9", ImGuiKey_9},
    {"F1", ImGuiKey_F1}, {"F2", ImGuiKey_F2}, {"F3", ImGuiKey_F3}, {"F4", ImGuiKey_F4},
    {"F5", ImGuiKey_F5}, {"F6", ImGuiKey_F6}, {"F7", ImGuiKey_F7}, {"F8", ImGuiKey_F8},
    {"F9", ImGuiKey_F9}, {"F10", ImGuiKey_F10}, {"F11", ImGuiKey_F11}, {"F12", ImGuiKey_F12},
    {"Space", ImGuiKey_Space}, {"Enter", ImGuiKey_Enter}, {"Return", ImGuiKey_Enter},
    {"Tab", ImGuiKey_Tab}, {"Escape", ImGuiKey_Escape}, {"Esc", ImGuiKey_Escape},
    {"Backspace", ImGuiKey_Backspace}, {"Delete", ImGuiKey_Delete}, {"Del", ImGuiKey_Delete},
    {"Insert", ImGuiKey_Insert}, {"Home", ImGuiKey_Home}, {"End", ImGuiKey_End},
    {"PageUp", ImGuiKey_PageUp}, {"PageDown", ImGuiKey_PageDown},
    {"Left", ImGuiKey_LeftArrow}, {"Right", ImGuiKey_RightArrow},
    {"Up", ImGuiKey_UpArrow}, {"Down", ImGuiKey_DownArrow},
    {"-", ImGuiKey_Minus}, {"=", ImGuiKey_Equal}, {"+", ImGuiKey_Equal},
    {"[", ImGuiKey_LeftBracket}, {"]", ImGuiKey_RightBracket},
    {";", ImGuiKey_Semicolon}, {"'", ImGuiKey_Apostrophe},
    {",", ImGuiKey_Comma}, {".", ImGuiKey_Period}, {"/", ImGuiKey_Slash},
    {"\\", ImGuiKey_Backslash}, {"`", ImGuiKey_GraveAccent},
};

ImGuiKey parseKeyName(const std::string& name)
{
    std::string upper = name;
    for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    
    auto it = kKeyNameMap.find(upper);
    if (it != kKeyNameMap.end()) return it->second;
    
    // Try single character
    if (name.size() == 1)
    {
        char c = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
        if (c >= 'A' && c <= 'Z')
        {
            return static_cast<ImGuiKey>(ImGuiKey_A + (c - 'A'));
        }
        if (c >= '0' && c <= '9')
        {
            return static_cast<ImGuiKey>(ImGuiKey_0 + (c - '0'));
        }
    }
    
    return ImGuiKey_None;
}

std::string getKeyName(ImGuiKey key)
{
    if (key >= ImGuiKey_A && key <= ImGuiKey_Z)
    {
        return std::string(1, static_cast<char>('A' + (key - ImGuiKey_A)));
    }
    if (key >= ImGuiKey_0 && key <= ImGuiKey_9)
    {
        return std::string(1, static_cast<char>('0' + (key - ImGuiKey_0)));
    }
    if (key >= ImGuiKey_F1 && key <= ImGuiKey_F12)
    {
        return "F" + std::to_string(key - ImGuiKey_F1 + 1);
    }
    
    switch (key)
    {
        case ImGuiKey_Space: return "Space";
        case ImGuiKey_Enter: return "Enter";
        case ImGuiKey_Tab: return "Tab";
        case ImGuiKey_Escape: return "Esc";
        case ImGuiKey_Backspace: return "Backspace";
        case ImGuiKey_Delete: return "Delete";
        case ImGuiKey_Insert: return "Insert";
        case ImGuiKey_Home: return "Home";
        case ImGuiKey_End: return "End";
        case ImGuiKey_PageUp: return "PageUp";
        case ImGuiKey_PageDown: return "PageDown";
        case ImGuiKey_LeftArrow: return "Left";
        case ImGuiKey_RightArrow: return "Right";
        case ImGuiKey_UpArrow: return "Up";
        case ImGuiKey_DownArrow: return "Down";
        case ImGuiKey_Minus: return "-";
        case ImGuiKey_Equal: return "=";
        default: return "";
    }
}

std::string Shortcut::toString() const
{
    if (!isValid()) return "";
    
    std::string result;
    
#ifdef __APPLE__
    if (hasFlag(modifiers, KeyMod::Super)) result += "Cmd+";
    if (hasFlag(modifiers, KeyMod::Ctrl)) result += "Ctrl+";
#else
    if (hasFlag(modifiers, KeyMod::Ctrl)) result += "Ctrl+";
    if (hasFlag(modifiers, KeyMod::Super)) result += "Win+";
#endif
    if (hasFlag(modifiers, KeyMod::Alt)) result += "Alt+";
    if (hasFlag(modifiers, KeyMod::Shift)) result += "Shift+";
    
    result += getKeyName(key);
    return result;
}

Shortcut Shortcut::fromString(const std::string& str)
{
    Shortcut result;
    std::string remaining = str;
    
    // Parse modifiers
    auto parseModifier = [&remaining](const std::string& mod, KeyMod flag) -> KeyMod
    {
        auto pos = remaining.find(mod + "+");
        if (pos != std::string::npos)
        {
            remaining.erase(pos, mod.size() + 1);
            return flag;
        }
        return KeyMod::None;
    };
    
    result.modifiers = result.modifiers | parseModifier("Ctrl", KeyMod::Ctrl);
    result.modifiers = result.modifiers | parseModifier("Cmd", KeyMod::Super);
    result.modifiers = result.modifiers | parseModifier("Win", KeyMod::Super);
    result.modifiers = result.modifiers | parseModifier("Alt", KeyMod::Alt);
    result.modifiers = result.modifiers | parseModifier("Shift", KeyMod::Shift);
    
    // Parse key
    result.key = parseKeyName(remaining);
    
    return result;
}

Shortcuts::Shortcuts()
{
    std::memset(searchBuffer_, 0, sizeof(searchBuffer_));
}

bool Shortcuts::registerCommand(
    const std::string& id,
    const std::string& name,
    const std::string& category,
    const Shortcut& shortcut,
    std::function<void()> action,
    const std::string& description)
{
    // Check if already registered
    if (commandIndex_.find(id) != commandIndex_.end())
    {
        return false;
    }
    
    Command cmd;
    cmd.id = id;
    cmd.name = name;
    cmd.category = category;
    cmd.description = description;
    cmd.shortcut = shortcut;
    cmd.action = std::move(action);
    cmd.enabled = true;
    
    commandIndex_[id] = commands_.size();
    commands_.push_back(std::move(cmd));
    needsSearchUpdate_ = true;
    
    return true;
}

void Shortcuts::unregisterCommand(const std::string& id)
{
    auto it = commandIndex_.find(id);
    if (it == commandIndex_.end()) return;
    
    size_t idx = it->second;
    commands_.erase(commands_.begin() + static_cast<long>(idx));
    commandIndex_.erase(it);
    
    // Update indices
    for (auto& [cmdId, cmdIdx] : commandIndex_)
    {
        if (cmdIdx > idx) cmdIdx--;
    }
    needsSearchUpdate_ = true;
}

bool Shortcuts::remapShortcut(const std::string& id, const Shortcut& newShortcut)
{
    auto it = commandIndex_.find(id);
    if (it == commandIndex_.end()) return false;
    
    // Check for conflicts
    std::string conflict = getConflict(newShortcut, id);
    if (!conflict.empty()) return false;
    
    commands_[it->second].shortcut = newShortcut;
    return true;
}

void Shortcuts::clearShortcut(const std::string& id)
{
    auto it = commandIndex_.find(id);
    if (it == commandIndex_.end()) return;
    
    commands_[it->second].shortcut = Shortcut{};
}

std::string Shortcuts::getConflict(const Shortcut& shortcut, const std::string& excludeId) const
{
    if (!shortcut.isValid()) return "";
    
    for (const auto& cmd : commands_)
    {
        if (cmd.id == excludeId) continue;
        if (cmd.shortcut == shortcut) return cmd.id;
    }
    return "";
}

const Command* Shortcuts::getCommand(const std::string& id) const
{
    auto it = commandIndex_.find(id);
    if (it == commandIndex_.end()) return nullptr;
    return &commands_[it->second];
}

std::vector<const Command*> Shortcuts::getCommandsByCategory(const std::string& category) const
{
    std::vector<const Command*> result;
    for (const auto& cmd : commands_)
    {
        if (cmd.category == category)
        {
            result.push_back(&cmd);
        }
    }
    return result;
}

int Shortcuts::fuzzyScore(const std::string& query, const std::string& text)
{
    if (query.empty()) return 1;
    
    std::string lowerQuery = query;
    std::string lowerText = text;
    for (auto& c : lowerQuery) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (auto& c : lowerText) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    
    // Exact match
    if (lowerText == lowerQuery) return 1000;
    
    // Starts with
    if (lowerText.find(lowerQuery) == 0) return 500;
    
    // Contains
    if (lowerText.find(lowerQuery) != std::string::npos) return 100;
    
    // Fuzzy match - check if all query chars appear in order
    size_t qi = 0;
    int score = 0;
    bool prevMatch = false;
    
    for (size_t ti = 0; ti < lowerText.size() && qi < lowerQuery.size(); ++ti)
    {
        if (lowerText[ti] == lowerQuery[qi])
        {
            score += prevMatch ? 5 : 1;
            prevMatch = true;
            qi++;
        }
        else
        {
            prevMatch = false;
        }
    }
    
    return (qi == lowerQuery.size()) ? score : 0;
}

std::vector<const Command*> Shortcuts::search(const std::string& query, size_t maxResults) const
{
    std::vector<std::pair<int, const Command*>> scored;
    
    for (const auto& cmd : commands_)
    {
        if (!cmd.enabled) continue;
        
        int nameScore = fuzzyScore(query, cmd.name);
        int catScore = fuzzyScore(query, cmd.category) / 2;
        int descScore = fuzzyScore(query, cmd.description) / 4;
        int idScore = fuzzyScore(query, cmd.id) / 2;
        
        int totalScore = std::max({nameScore, catScore, descScore, idScore});
        
        if (totalScore > 0)
        {
            scored.emplace_back(totalScore, &cmd);
        }
    }
    
    std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });
    
    std::vector<const Command*> result;
    for (size_t i = 0; i < std::min(maxResults, scored.size()); ++i)
    {
        result.push_back(scored[i].second);
    }
    
    return result;
}

Shortcut Shortcuts::getCurrentModifiers() const
{
    Shortcut result;
    if (ImGui::GetIO().KeyCtrl) result.modifiers = result.modifiers | KeyMod::Ctrl;
    if (ImGui::GetIO().KeyShift) result.modifiers = result.modifiers | KeyMod::Shift;
    if (ImGui::GetIO().KeyAlt) result.modifiers = result.modifiers | KeyMod::Alt;
    if (ImGui::GetIO().KeySuper) result.modifiers = result.modifiers | KeyMod::Super;
    return result;
}

bool Shortcuts::isShortcutPressed(const Shortcut& shortcut) const
{
    if (!shortcut.isValid()) return false;
    
    // Check key pressed
    if (!ImGui::IsKeyPressed(shortcut.key, false)) return false;
    
    // Check modifiers match exactly
    bool ctrlOk = hasFlag(shortcut.modifiers, KeyMod::Ctrl) == ImGui::GetIO().KeyCtrl;
    bool shiftOk = hasFlag(shortcut.modifiers, KeyMod::Shift) == ImGui::GetIO().KeyShift;
    bool altOk = hasFlag(shortcut.modifiers, KeyMod::Alt) == ImGui::GetIO().KeyAlt;
    bool superOk = hasFlag(shortcut.modifiers, KeyMod::Super) == ImGui::GetIO().KeySuper;
    
    return ctrlOk && shiftOk && altOk && superOk;
}

void Shortcuts::processInput()
{
    // Check for command palette shortcut (Ctrl+K or Cmd+K)
    Shortcut paletteShortcut{ImGuiKey_K, KeyMod::Ctrl};
#ifdef __APPLE__
    paletteShortcut.modifiers = KeyMod::Super;
#endif
    
    if (isShortcutPressed(paletteShortcut))
    {
        commandPaletteOpen_ = !commandPaletteOpen_;
        if (commandPaletteOpen_)
        {
            std::memset(searchBuffer_, 0, sizeof(searchBuffer_));
            selectedIndex_ = 0;
            needsSearchUpdate_ = true;
        }
        return;
    }
    
    // Don't process other shortcuts while palette is open
    if (commandPaletteOpen_) return;
    
    // Check all registered shortcuts
    for (const auto& cmd : commands_)
    {
        if (!cmd.enabled) continue;
        if (!cmd.shortcut.isValid()) continue;
        
        if (isShortcutPressed(cmd.shortcut))
        {
            if (cmd.action) cmd.action();
            break;
        }
    }
}

bool Shortcuts::executeCommand(const std::string& id)
{
    const Command* cmd = getCommand(id);
    if (!cmd || !cmd->enabled || !cmd->action) return false;
    
    cmd->action();
    return true;
}

void Shortcuts::setCommandEnabled(const std::string& id, bool enabled)
{
    auto it = commandIndex_.find(id);
    if (it == commandIndex_.end()) return;
    commands_[it->second].enabled = enabled;
}

void Shortcuts::updateSearch()
{
    searchResults_ = search(searchBuffer_, 20);
    selectedIndex_ = 0;
}

void Shortcuts::drawCommandPalette(bool& open)
{
    if (!open) return;
    
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 center = viewport->GetCenter();
    ImGui::SetNextWindowPos(ImVec2(center.x, center.y * 0.4f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Appearing);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    
    if (ImGui::Begin("##CommandPalette", &open, flags))
    {
        // Search input
        ImGui::PushItemWidth(-1);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 10));
        
        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere();
        }
        
        bool inputChanged = ImGui::InputTextWithHint("##Search", "Type to search commands...", 
                                                      searchBuffer_, sizeof(searchBuffer_));
        
        ImGui::PopStyleVar();
        ImGui::PopItemWidth();
        
        if (inputChanged)
        {
            needsSearchUpdate_ = true;
        }
        
        if (needsSearchUpdate_)
        {
            updateSearch();
            needsSearchUpdate_ = false;
        }
        
        // Handle keyboard navigation
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        {
            selectedIndex_ = std::min(selectedIndex_ + 1, static_cast<int>(searchResults_.size()) - 1);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
        {
            selectedIndex_ = std::max(selectedIndex_ - 1, 0);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            open = false;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) && !searchResults_.empty() && 
            selectedIndex_ < static_cast<int>(searchResults_.size()))
        {
            const Command* cmd = searchResults_[static_cast<size_t>(selectedIndex_)];
            if (cmd && cmd->action)
            {
                cmd->action();
            }
            open = false;
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Results list
        if (ImGui::BeginChild("##Results", ImVec2(0, 0), false))
        {
            for (size_t i = 0; i < searchResults_.size(); ++i)
            {
                const Command* cmd = searchResults_[i];
                bool isSelected = (static_cast<int>(i) == selectedIndex_);
                
                ImGui::PushID(static_cast<int>(i));
                
                if (isSelected)
                {
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                }
                
                if (ImGui::Selectable("##item", isSelected, 0, ImVec2(0, 36)))
                {
                    if (cmd->action) cmd->action();
                    open = false;
                }
                
                if (isSelected)
                {
                    ImGui::PopStyleColor();
                }
                
                // Draw command content on same line
                ImGui::SameLine(8);
                ImGui::BeginGroup();
                
                // Category badge
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 0.5f));
                ImGui::SmallButton(cmd->category.c_str());
                ImGui::PopStyleColor();
                
                ImGui::SameLine();
                ImGui::Text("%s", cmd->name.c_str());
                
                // Shortcut on right side
                if (cmd->shortcut.isValid())
                {
                    std::string shortcutStr = cmd->shortcut.toString();
                    float width = ImGui::CalcTextSize(shortcutStr.c_str()).x;
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - width - 8);
                    ImGui::TextDisabled("%s", shortcutStr.c_str());
                }
                
                ImGui::EndGroup();
                ImGui::PopID();
            }
            
            if (searchResults_.empty() && searchBuffer_[0] != '\0')
            {
                ImGui::TextDisabled("No matching commands");
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
    
    ImGui::PopStyleVar(2);
    
    // Handle click outside to close
    if (ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
    {
        open = false;
    }
}

void Shortcuts::registerDefaultCommands()
{
    // File commands
    registerCommand("file.new", "New Project", "File", 
        {ImGuiKey_N, KeyMod::Ctrl}, []() { /* TODO */ }, "Create a new project");
    registerCommand("file.open", "Open Project", "File", 
        {ImGuiKey_O, KeyMod::Ctrl}, []() { /* TODO */ }, "Open an existing project");
    registerCommand("file.save", "Save Project", "File", 
        {ImGuiKey_S, KeyMod::Ctrl}, []() { /* TODO */ }, "Save the current project");
    registerCommand("file.save_as", "Save As...", "File", 
        {ImGuiKey_S, KeyMod::Ctrl | KeyMod::Shift}, []() { /* TODO */ }, "Save project with a new name");
    registerCommand("file.export", "Export Audio", "File", 
        {ImGuiKey_E, KeyMod::Ctrl | KeyMod::Shift}, []() { /* TODO */ }, "Export project to audio file");
    
    // Edit commands
    registerCommand("edit.undo", "Undo", "Edit", 
        {ImGuiKey_Z, KeyMod::Ctrl}, []() { /* TODO */ }, "Undo last action");
    registerCommand("edit.redo", "Redo", "Edit", 
        {ImGuiKey_Y, KeyMod::Ctrl}, []() { /* TODO */ }, "Redo last undone action");
    registerCommand("edit.cut", "Cut", "Edit", 
        {ImGuiKey_X, KeyMod::Ctrl}, []() { /* TODO */ }, "Cut selection to clipboard");
    registerCommand("edit.copy", "Copy", "Edit", 
        {ImGuiKey_C, KeyMod::Ctrl}, []() { /* TODO */ }, "Copy selection to clipboard");
    registerCommand("edit.paste", "Paste", "Edit", 
        {ImGuiKey_V, KeyMod::Ctrl}, []() { /* TODO */ }, "Paste from clipboard");
    registerCommand("edit.delete", "Delete", "Edit", 
        {ImGuiKey_Delete, KeyMod::None}, []() { /* TODO */ }, "Delete selection");
    registerCommand("edit.select_all", "Select All", "Edit", 
        {ImGuiKey_A, KeyMod::Ctrl}, []() { /* TODO */ }, "Select all items");
    
    // Transport commands
    registerCommand("transport.play", "Play/Pause", "Transport", 
        {ImGuiKey_Space, KeyMod::None}, []() { /* TODO */ }, "Toggle playback");
    registerCommand("transport.stop", "Stop", "Transport", 
        {ImGuiKey_Enter, KeyMod::None}, []() { /* TODO */ }, "Stop playback");
    registerCommand("transport.record", "Record", "Transport", 
        {ImGuiKey_R, KeyMod::Ctrl}, []() { /* TODO */ }, "Toggle recording");
    registerCommand("transport.loop", "Toggle Loop", "Transport", 
        {ImGuiKey_L, KeyMod::Ctrl}, []() { /* TODO */ }, "Toggle loop mode");
    registerCommand("transport.goto_start", "Go to Start", "Transport", 
        {ImGuiKey_Home, KeyMod::None}, []() { /* TODO */ }, "Move playhead to start");
    registerCommand("transport.goto_end", "Go to End", "Transport", 
        {ImGuiKey_End, KeyMod::None}, []() { /* TODO */ }, "Move playhead to end");
    
    // View commands
    registerCommand("view.mixer", "Show Mixer", "View", 
        {ImGuiKey_M, KeyMod::Ctrl}, []() { /* TODO */ }, "Show/hide mixer panel");
    registerCommand("view.piano_roll", "Show Piano Roll", "View", 
        {ImGuiKey_P, KeyMod::Ctrl}, []() { /* TODO */ }, "Show/hide piano roll");
    registerCommand("view.browser", "Show Browser", "View", 
        {ImGuiKey_B, KeyMod::Ctrl}, []() { /* TODO */ }, "Show/hide browser panel");
    registerCommand("view.fullscreen", "Toggle Fullscreen", "View", 
        {ImGuiKey_F11, KeyMod::None}, []() { /* TODO */ }, "Toggle fullscreen mode");
    
    // Zoom commands
    registerCommand("zoom.in", "Zoom In", "Zoom", 
        {ImGuiKey_Equal, KeyMod::Ctrl}, []() { /* TODO */ }, "Zoom in on timeline");
    registerCommand("zoom.out", "Zoom Out", "Zoom", 
        {ImGuiKey_Minus, KeyMod::Ctrl}, []() { /* TODO */ }, "Zoom out on timeline");
    registerCommand("zoom.fit", "Zoom to Fit", "Zoom", 
        {ImGuiKey_0, KeyMod::Ctrl}, []() { /* TODO */ }, "Fit content in view");
}

bool Shortcuts::loadFromFile(const std::filesystem::path& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) return false;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    
    // Simple JSON parsing for shortcut remappings
    // Format: { "command_id": "Ctrl+Key", ... }
    for (auto& cmd : commands_)
    {
        std::string searchKey = "\"" + cmd.id + "\"";
        auto pos = json.find(searchKey);
        if (pos == std::string::npos) continue;
        
        pos = json.find(':', pos);
        if (pos == std::string::npos) continue;
        
        pos = json.find('"', pos);
        if (pos == std::string::npos) continue;
        
        auto endPos = json.find('"', pos + 1);
        if (endPos == std::string::npos) continue;
        
        std::string shortcutStr = json.substr(pos + 1, endPos - pos - 1);
        cmd.shortcut = Shortcut::fromString(shortcutStr);
    }
    
    return true;
}

bool Shortcuts::saveToFile(const std::filesystem::path& filepath) const
{
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    
    file << "{\n";
    bool first = true;
    for (const auto& cmd : commands_)
    {
        if (!first) file << ",\n";
        first = false;
        file << "  \"" << cmd.id << "\": \"" << cmd.shortcut.toString() << "\"";
    }
    file << "\n}\n";
    
    return true;
}

} // namespace daw::ui::imgui
