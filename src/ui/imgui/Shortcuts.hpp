#pragma once

#include "imgui.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <filesystem>

namespace daw::ui::imgui
{

/**
 * @brief Key modifier flags
 */
enum class KeyMod : uint32_t
{
    None = 0,
    Ctrl = 1 << 0,
    Shift = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3,  // Cmd on macOS, Win on Windows
};

inline KeyMod operator|(KeyMod a, KeyMod b)
{
    return static_cast<KeyMod>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline KeyMod operator&(KeyMod a, KeyMod b)
{
    return static_cast<KeyMod>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool hasFlag(KeyMod mods, KeyMod flag)
{
    return (static_cast<uint32_t>(mods) & static_cast<uint32_t>(flag)) != 0;
}

/**
 * @brief Keyboard shortcut definition
 */
struct Shortcut
{
    ImGuiKey key{ImGuiKey_None};
    KeyMod modifiers{KeyMod::None};

    bool operator==(const Shortcut& other) const
    {
        return key == other.key && modifiers == other.modifiers;
    }

    [[nodiscard]] bool isValid() const { return key != ImGuiKey_None; }

    [[nodiscard]] std::string toString() const;
    static Shortcut fromString(const std::string& str);
};

/**
 * @brief Command definition with associated shortcut
 */
struct Command
{
    std::string id;
    std::string name;
    std::string category;
    std::string description;
    Shortcut shortcut;
    std::function<void()> action;
    bool enabled{true};
};

/**
 * @brief Shortcut registry and command palette manager
 * 
 * Manages keyboard shortcuts with conflict detection,
 * runtime remapping, and fuzzy search for command palette.
 */
class Shortcuts
{
public:
    Shortcuts();
    ~Shortcuts() = default;

    // Non-copyable
    Shortcuts(const Shortcuts&) = delete;
    Shortcuts& operator=(const Shortcuts&) = delete;
    Shortcuts(Shortcuts&&) = default;
    Shortcuts& operator=(Shortcuts&&) = default;

    /**
     * @brief Register a new command with shortcut
     * @param id Unique command identifier (e.g., "file.save")
     * @param name Display name (e.g., "Save File")
     * @param category Category for grouping (e.g., "File")
     * @param shortcut Default keyboard shortcut
     * @param action Function to execute
     * @param description Optional description
     * @return true if registered successfully
     */
    bool registerCommand(
        const std::string& id,
        const std::string& name,
        const std::string& category,
        const Shortcut& shortcut,
        std::function<void()> action,
        const std::string& description = ""
    );

    /**
     * @brief Unregister a command
     * @param id Command identifier
     */
    void unregisterCommand(const std::string& id);

    /**
     * @brief Remap a shortcut for a command
     * @param id Command identifier
     * @param newShortcut New shortcut to assign
     * @return true if remapped successfully
     */
    bool remapShortcut(const std::string& id, const Shortcut& newShortcut);

    /**
     * @brief Clear shortcut for a command
     * @param id Command identifier
     */
    void clearShortcut(const std::string& id);

    /**
     * @brief Check for shortcut conflicts
     * @param shortcut Shortcut to check
     * @param excludeId Command ID to exclude from check
     * @return ID of conflicting command, or empty string
     */
    [[nodiscard]] std::string getConflict(const Shortcut& shortcut, const std::string& excludeId = "") const;

    /**
     * @brief Get command by ID
     * @param id Command identifier
     * @return Pointer to command or nullptr
     */
    [[nodiscard]] const Command* getCommand(const std::string& id) const;

    /**
     * @brief Get all commands
     */
    [[nodiscard]] const std::vector<Command>& getCommands() const { return commands_; }

    /**
     * @brief Get commands in a category
     */
    [[nodiscard]] std::vector<const Command*> getCommandsByCategory(const std::string& category) const;

    /**
     * @brief Search commands with fuzzy matching
     * @param query Search query
     * @param maxResults Maximum results to return
     * @return Matching commands sorted by relevance
     */
    [[nodiscard]] std::vector<const Command*> search(const std::string& query, size_t maxResults = 20) const;

    /**
     * @brief Process keyboard input and execute matching commands
     * Should be called once per frame.
     */
    void processInput();

    /**
     * @brief Execute a command by ID
     * @param id Command identifier
     * @return true if command was executed
     */
    bool executeCommand(const std::string& id);

    /**
     * @brief Enable/disable a command
     * @param id Command identifier
     * @param enabled Whether command is enabled
     */
    void setCommandEnabled(const std::string& id, bool enabled);

    /**
     * @brief Load shortcuts from JSON file
     * @param filepath Path to shortcuts JSON file
     * @return true if loaded successfully
     */
    bool loadFromFile(const std::filesystem::path& filepath);

    /**
     * @brief Save shortcuts to JSON file
     * @param filepath Path to save shortcuts JSON file
     * @return true if saved successfully
     */
    bool saveToFile(const std::filesystem::path& filepath) const;

    /**
     * @brief Register default DAW commands
     */
    void registerDefaultCommands();

    /**
     * @brief Draw command palette UI
     * @param open Reference to bool controlling visibility
     */
    void drawCommandPalette(bool& open);

    /**
     * @brief Check if command palette is currently open
     */
    [[nodiscard]] bool isCommandPaletteOpen() const { return commandPaletteOpen_; }

    /**
     * @brief Open command palette
     */
    void openCommandPalette() { commandPaletteOpen_ = true; }

    /**
     * @brief Close command palette
     */
    void closeCommandPalette() { commandPaletteOpen_ = false; }

private:
    std::vector<Command> commands_;
    std::unordered_map<std::string, size_t> commandIndex_;
    bool commandPaletteOpen_{false};
    char searchBuffer_[256]{};
    int selectedIndex_{0};
    std::vector<const Command*> searchResults_;
    bool needsSearchUpdate_{true};

    void updateSearch();
    static int fuzzyScore(const std::string& query, const std::string& text);
    [[nodiscard]] Shortcut getCurrentModifiers() const;
    [[nodiscard]] bool isShortcutPressed(const Shortcut& shortcut) const;
};

/**
 * @brief Parse key name to ImGuiKey
 */
ImGuiKey parseKeyName(const std::string& name);

/**
 * @brief Get display name for ImGuiKey
 */
std::string getKeyName(ImGuiKey key);

} // namespace daw::ui::imgui
