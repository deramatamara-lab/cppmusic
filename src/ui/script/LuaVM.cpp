/**
 * @file LuaVM.cpp
 * @brief Lua VM sandbox implementation
 * 
 * Note: This is a stub implementation that provides the interface
 * without requiring the Lua library. When ENABLE_LUA_SCRIPTING is
 * enabled in CMake, this will be replaced with full Lua integration.
 */

#include "LuaVM.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace daw::ui::script
{

// ============================================================================
// LuaVM Implementation (Stub without actual Lua)
// ============================================================================

LuaVM::LuaVM() = default;

LuaVM::~LuaVM()
{
    shutdown();
}

bool LuaVM::initialize()
{
    if (state_ != nullptr) {
        return true;  // Already initialized
    }
    
    // In stub mode, we just mark as initialized
    // Real implementation would call luaL_newstate()
    state_ = reinterpret_cast<lua_State*>(1);  // Non-null marker
    
    setupSandbox();
    setupAPI();
    
    return true;
}

void LuaVM::shutdown()
{
    if (state_ != nullptr) {
        // Real implementation would call lua_close()
        state_ = nullptr;
    }
    
    actions_.clear();
    panels_.clear();
    paramSubscriptions_.clear();
    currentMemory_ = 0;
    instructionCounter_ = 0;
}

ScriptResult LuaVM::execute(const std::string& script, const std::string& chunkName)
{
    ScriptResult result;
    
    if (!isInitialized()) {
        result.success = false;
        result.error = "Lua VM not initialized";
        return result;
    }
    
    // Validate script (basic security check)
    if (script.find("os.execute") != std::string::npos && !security_.allowSystemCalls) {
        result.success = false;
        result.error = "Security violation: os.execute is not allowed";
        return result;
    }
    
    if (script.find("io.") != std::string::npos && !security_.allowFileRead && !security_.allowFileWrite) {
        result.success = false;
        result.error = "Security violation: file I/O is not allowed";
        return result;
    }
    
    if (script.find("loadstring") != std::string::npos && !security_.allowLoadstring) {
        result.success = false;
        result.error = "Security violation: loadstring is not allowed";
        return result;
    }
    
    // Stub: Log script execution
    std::cout << "[LuaVM] Executing chunk '" << chunkName << "' (" 
              << script.size() << " bytes)\n";
    
    // In real implementation:
    // 1. Set instruction hook for limits
    // 2. Load and execute the script
    // 3. Collect return values
    
    result.success = true;
    result.instructionCount = 0;
    result.memoryUsed = currentMemory_;
    lastInstructionCount_ = 0;
    
    return result;
}

ScriptResult LuaVM::executeFile(const std::string& filepath)
{
    ScriptResult result;
    
    // Security: Only allow files in extensions directory
    std::filesystem::path fullPath = security_.extensionsDirectory;
    fullPath /= filepath;
    
    // Validate path doesn't escape extensions directory
    auto canonical = std::filesystem::weakly_canonical(fullPath);
    auto basePath = std::filesystem::weakly_canonical(security_.extensionsDirectory);
    
    std::string canonicalStr = canonical.string();
    std::string baseStr = basePath.string();
    
    if (canonicalStr.find(baseStr) != 0) {
        result.success = false;
        result.error = "Security violation: path escapes extensions directory";
        return result;
    }
    
    // Read file
    std::ifstream file(canonical);
    if (!file.is_open()) {
        result.success = false;
        result.error = "Failed to open file: " + filepath;
        return result;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return execute(buffer.str(), filepath);
}

ScriptResult LuaVM::callFunction(const std::string& funcName,
                                  const std::vector<LuaValue>& args)
{
    ScriptResult result;
    
    if (!isInitialized()) {
        result.success = false;
        result.error = "Lua VM not initialized";
        return result;
    }
    
    // Stub: Log function call
    std::cout << "[LuaVM] Calling function '" << funcName << "' with " 
              << args.size() << " arguments\n";
    
    result.success = true;
    return result;
}

void LuaVM::setGlobal(const std::string& name, const LuaValue& value)
{
    if (!isInitialized()) return;
    
    // Stub: Just log
    std::cout << "[LuaVM] Setting global '" << name << "'\n";
    (void)value;
}

std::optional<LuaValue> LuaVM::getGlobal(const std::string& name)
{
    if (!isInitialized()) return std::nullopt;
    
    // Stub: Return nil
    std::cout << "[LuaVM] Getting global '" << name << "'\n";
    return std::nullopt;
}

void LuaVM::reset()
{
    shutdown();
    initialize();
}

std::size_t LuaVM::getMemoryUsage() const
{
    return currentMemory_;
}

void LuaVM::setupSandbox()
{
    // In real implementation:
    // 1. Remove dangerous functions (os.execute, io.*, debug.*, etc.)
    // 2. Set up custom allocator for memory tracking
    // 3. Set up instruction hook for CPU limits
}

void LuaVM::setupAPI()
{
    // Register safe API functions
    // In real implementation, these would be registered with lua_register()
}

// ============================================================================
// ExtensionAPI Implementation
// ============================================================================

LuaVM& ExtensionAPI::getVM()
{
    return getGlobalLuaVM();
}

int ExtensionAPI::loadAllScripts(const std::string& directory)
{
    auto& vm = getVM();
    if (!vm.isInitialized()) {
        if (!vm.initialize()) {
            return 0;
        }
    }
    
    int count = 0;
    
    if (!std::filesystem::exists(directory)) {
        return 0;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            auto relativePath = std::filesystem::relative(entry.path(), directory);
            auto result = vm.executeFile(relativePath.string());
            if (result.success) {
                count++;
            } else {
                std::cerr << "[ExtensionAPI] Failed to load " << entry.path() 
                          << ": " << result.error << "\n";
            }
        }
    }
    
    return count;
}

bool ExtensionAPI::reloadScript(const std::string& filepath)
{
    auto& vm = getVM();
    auto result = vm.executeFile(filepath);
    return result.success;
}

bool ExtensionAPI::executeAction(const std::string& actionId)
{
    auto& vm = getVM();
    const auto& actions = vm.getActions();
    
    for (const auto& action : actions) {
        if (action.id == actionId && action.callback) {
            action.callback();
            return true;
        }
    }
    
    return false;
}

void ExtensionAPI::drawPanels()
{
    auto& vm = getVM();
    const auto& panels = vm.getPanels();
    
    for (const auto& panel : panels) {
        if (panel.visible && panel.drawCallback) {
            panel.drawCallback();
        }
    }
}

std::vector<LuaAction> ExtensionAPI::getAvailableActions()
{
    return getVM().getActions();
}

} // namespace daw::ui::script
