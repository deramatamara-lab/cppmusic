/**
 * @file LuaVM.hpp
 * @brief Lua scripting sandbox with resource limits and security
 */
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// Forward declare Lua state to avoid header inclusion
struct lua_State;

namespace daw::ui::script
{

/**
 * @brief Lua value variant type
 */
using LuaValue = std::variant<
    std::nullptr_t,
    bool,
    double,
    std::string,
    std::vector<std::pair<std::string, std::string>>  // Table as key-value pairs
>;

/**
 * @brief Script execution result
 */
struct ScriptResult
{
    bool success{false};
    std::string error;
    std::vector<LuaValue> returnValues;
    uint64_t instructionCount{0};
    std::size_t memoryUsed{0};
};

/**
 * @brief Resource limits for Lua sandbox
 */
struct LuaLimits
{
    uint64_t maxInstructions{1000000};   // Max instructions per call
    std::size_t maxMemoryBytes{16 * 1024 * 1024};  // 16MB default
    int maxCallDepth{100};                // Max function call depth
    int maxStringLength{1024 * 1024};     // 1MB max string
    double maxExecutionTimeSeconds{5.0};  // Max wall-clock time
};

/**
 * @brief Security settings for Lua sandbox
 */
struct LuaSecurity
{
    bool allowFileRead{false};           // Allow reading files
    bool allowFileWrite{false};          // Allow writing files
    bool allowNetwork{false};            // Allow network access
    bool allowSystemCalls{false};        // Allow os.execute, etc.
    bool allowDebug{false};              // Allow debug library
    bool allowLoadstring{false};         // Allow dynamic code loading
    
    std::string extensionsDirectory{"scripts/"};  // Only allow file access here
};

/**
 * @brief Action registration for UI commands
 */
struct LuaAction
{
    std::string id;
    std::string name;
    std::string description;
    std::string shortcut;  // Optional keyboard shortcut
    std::string category;
    std::function<void()> callback;
};

/**
 * @brief Panel created by Lua script
 */
struct LuaPanel
{
    std::string id;
    std::string title;
    std::function<void()> drawCallback;
    bool visible{true};
};

/**
 * @brief Lua VM sandbox with security and resource limits
 */
class LuaVM
{
public:
    LuaVM();
    ~LuaVM();

    // Non-copyable
    LuaVM(const LuaVM&) = delete;
    LuaVM& operator=(const LuaVM&) = delete;

    /**
     * @brief Initialize the Lua VM
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Shutdown the Lua VM
     */
    void shutdown();

    /**
     * @brief Check if VM is initialized
     */
    [[nodiscard]] bool isInitialized() const { return state_ != nullptr; }

    /**
     * @brief Execute a Lua script string
     * @param script Lua code to execute
     * @param chunkName Name for error messages
     * @return Execution result
     */
    ScriptResult execute(const std::string& script, const std::string& chunkName = "chunk");

    /**
     * @brief Load and execute a Lua script file
     * @param filepath Path to script file (relative to extensions directory)
     * @return Execution result
     */
    ScriptResult executeFile(const std::string& filepath);

    /**
     * @brief Call a global Lua function
     * @param funcName Function name
     * @param args Arguments to pass
     * @return Execution result
     */
    ScriptResult callFunction(const std::string& funcName,
                              const std::vector<LuaValue>& args = {});

    /**
     * @brief Set a global variable
     */
    void setGlobal(const std::string& name, const LuaValue& value);

    /**
     * @brief Get a global variable
     */
    std::optional<LuaValue> getGlobal(const std::string& name);

    /**
     * @brief Get/set resource limits
     */
    [[nodiscard]] const LuaLimits& getLimits() const { return limits_; }
    void setLimits(const LuaLimits& limits) { limits_ = limits; }

    /**
     * @brief Get/set security settings
     */
    [[nodiscard]] const LuaSecurity& getSecurity() const { return security_; }
    void setSecurity(const LuaSecurity& security) { security_ = security; }

    /**
     * @brief Get registered actions
     */
    [[nodiscard]] const std::vector<LuaAction>& getActions() const { return actions_; }

    /**
     * @brief Get created panels
     */
    [[nodiscard]] const std::vector<LuaPanel>& getPanels() const { return panels_; }

    /**
     * @brief Expose API function to Lua
     * 
     * @param name Function name in Lua
     * @param func C++ function to call
     */
    template<typename Func>
    void exposeFunction(const std::string& name, Func&& func);

    /**
     * @brief Reset VM state (clear all scripts and state)
     */
    void reset();

    /**
     * @brief Get current memory usage
     */
    [[nodiscard]] std::size_t getMemoryUsage() const;

    /**
     * @brief Get instruction count from last execution
     */
    [[nodiscard]] uint64_t getLastInstructionCount() const { return lastInstructionCount_; }

private:
    void setupSandbox();
    void setupAPI();
    void hookFunction(lua_State* L, int ar);
    
    static int luaAllocator(void* ud, void* ptr, std::size_t osize, std::size_t nsize);
    static void luaHook(lua_State* L, void* ar);

    // API implementations
    static int api_register_action(lua_State* L);
    static int api_create_panel(lua_State* L);
    static int api_subscribe_param(lua_State* L);
    static int api_get_selection(lua_State* L);
    static int api_log(lua_State* L);
    static int api_get_transport_state(lua_State* L);
    static int api_set_transport_state(lua_State* L);

    lua_State* state_{nullptr};
    LuaLimits limits_;
    LuaSecurity security_;
    
    std::vector<LuaAction> actions_;
    std::vector<LuaPanel> panels_;
    std::unordered_map<std::string, std::function<void(double)>> paramSubscriptions_;
    
    std::size_t currentMemory_{0};
    uint64_t instructionCounter_{0};
    uint64_t lastInstructionCount_{0};
    bool executionAborted_{false};
};

/**
 * @brief Extension API exposed to Lua scripts
 */
class ExtensionAPI
{
public:
    /**
     * @brief Get the global Lua VM instance
     */
    static LuaVM& getVM();

    /**
     * @brief Load all scripts from extensions directory
     * @param directory Extensions directory path
     * @return Number of scripts loaded
     */
    static int loadAllScripts(const std::string& directory);

    /**
     * @brief Reload a specific script
     */
    static bool reloadScript(const std::string& filepath);

    /**
     * @brief Execute action by ID
     */
    static bool executeAction(const std::string& actionId);

    /**
     * @brief Draw all Lua-created panels
     */
    static void drawPanels();

    /**
     * @brief Get available actions for command palette
     */
    static std::vector<LuaAction> getAvailableActions();
};

/**
 * @brief Global Lua VM instance
 */
inline LuaVM& getGlobalLuaVM()
{
    static LuaVM instance;
    return instance;
}

} // namespace daw::ui::script
