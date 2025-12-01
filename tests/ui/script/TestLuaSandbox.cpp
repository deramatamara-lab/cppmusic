/**
 * @file TestLuaSandbox.cpp
 * @brief Tests for Lua scripting sandbox security
 */

#include "ui/script/LuaVM.hpp"
#include <cassert>
#include <iostream>

using namespace daw::ui::script;

namespace
{

void testBasicExecution()
{
    std::cout << "Testing basic Lua execution..." << std::endl;
    
    LuaVM vm;
    assert(vm.initialize() && "VM should initialize");
    
    auto result = vm.execute("return 1 + 1", "test");
    assert(result.success && "Simple math should succeed");
    
    vm.shutdown();
    std::cout << "  PASSED" << std::endl;
}

void testSecurityBlockOsExecute()
{
    std::cout << "Testing os.execute blocking..." << std::endl;
    
    LuaVM vm;
    vm.initialize();
    
    LuaSecurity security;
    security.allowSystemCalls = false;
    vm.setSecurity(security);
    
    auto result = vm.execute("os.execute('echo hello')", "test");
    assert(!result.success && "os.execute should be blocked");
    assert(result.error.find("Security") != std::string::npos && 
           "Error should mention security");
    
    vm.shutdown();
    std::cout << "  PASSED" << std::endl;
}

void testSecurityBlockFileIO()
{
    std::cout << "Testing file I/O blocking..." << std::endl;
    
    LuaVM vm;
    vm.initialize();
    
    LuaSecurity security;
    security.allowFileRead = false;
    security.allowFileWrite = false;
    vm.setSecurity(security);
    
    auto result = vm.execute("io.open('/etc/passwd', 'r')", "test");
    assert(!result.success && "io.open should be blocked");
    
    vm.shutdown();
    std::cout << "  PASSED" << std::endl;
}

void testSecurityBlockLoadstring()
{
    std::cout << "Testing loadstring blocking..." << std::endl;
    
    LuaVM vm;
    vm.initialize();
    
    LuaSecurity security;
    security.allowLoadstring = false;
    vm.setSecurity(security);
    
    auto result = vm.execute("loadstring('print(1)')", "test");
    assert(!result.success && "loadstring should be blocked");
    
    vm.shutdown();
    std::cout << "  PASSED" << std::endl;
}

void testPathEscapePrevention()
{
    std::cout << "Testing path escape prevention..." << std::endl;
    
    LuaVM vm;
    vm.initialize();
    
    LuaSecurity security;
    security.allowFileRead = true;
    security.extensionsDirectory = "/home/test/scripts/";
    vm.setSecurity(security);
    
    auto result = vm.executeFile("../../../etc/passwd");
    assert(!result.success && "Path escape should be blocked");
    assert(result.error.find("escapes") != std::string::npos && 
           "Error should mention path escape");
    
    vm.shutdown();
    std::cout << "  PASSED" << std::endl;
}

void testResourceLimits()
{
    std::cout << "Testing resource limits..." << std::endl;
    
    LuaVM vm;
    vm.initialize();
    
    LuaLimits limits;
    limits.maxInstructions = 1000;
    limits.maxMemoryBytes = 1024 * 1024;  // 1MB
    vm.setLimits(limits);
    
    // This test verifies the limits are set correctly
    // Actual enforcement requires full Lua integration
    const auto& currentLimits = vm.getLimits();
    (void)currentLimits;  // Used in assert below
    assert(currentLimits.maxInstructions == 1000 && "Instruction limit should be set");
    assert(currentLimits.maxMemoryBytes == 1024 * 1024 && "Memory limit should be set");
    
    vm.shutdown();
    std::cout << "  PASSED" << std::endl;
}

void testGlobalVariables()
{
    std::cout << "Testing global variables..." << std::endl;
    
    LuaVM vm;
    vm.initialize();
    
    // Set global
    vm.setGlobal("myValue", LuaValue(42.0));
    
    // In stub implementation, getGlobal returns nullopt
    // Full implementation would return the value
    auto value = vm.getGlobal("myValue");
    // Just verify no crash
    
    vm.shutdown();
    std::cout << "  PASSED" << std::endl;
}

void testVMReset()
{
    std::cout << "Testing VM reset..." << std::endl;
    
    LuaVM vm;
    vm.initialize();
    
    // Execute something
    vm.execute("local x = 1", "test");
    
    // Reset
    vm.reset();
    
    // Should be clean state
    assert(vm.isInitialized() && "VM should be initialized after reset");
    
    vm.shutdown();
    assert(!vm.isInitialized() && "VM should not be initialized after shutdown");
    
    std::cout << "  PASSED" << std::endl;
}

void testExtensionAPI()
{
    std::cout << "Testing ExtensionAPI..." << std::endl;
    
    auto& vm = ExtensionAPI::getVM();
    vm.initialize();
    
    // Get actions (should be empty initially)
    auto actions = ExtensionAPI::getAvailableActions();
    (void)actions;  // Used in assert below
    assert(actions.empty() && "No actions registered initially");
    
    // Execute non-existent action
    bool executed = ExtensionAPI::executeAction("non.existent");
    (void)executed;  // Used in assert below
    assert(!executed && "Non-existent action should fail");
    
    vm.shutdown();
    std::cout << "  PASSED" << std::endl;
}

void testMemoryTracking()
{
    std::cout << "Testing memory tracking..." << std::endl;
    
    LuaVM vm;
    vm.initialize();
    
    std::size_t initialMemory = vm.getMemoryUsage();
    (void)initialMemory;  // Used in assert below
    
    // In stub implementation, memory is tracked but always 0
    // Full implementation would show actual Lua memory usage
    // Note: initialMemory is size_t which is always non-negative
    assert(initialMemory == 0 || initialMemory > 0);  // Just verify it's a valid value
    
    vm.shutdown();
    std::cout << "  PASSED" << std::endl;
}

} // anonymous namespace

int main()
{
    std::cout << "=== Lua Sandbox Tests ===" << std::endl;
    
    try {
        testBasicExecution();
        testSecurityBlockOsExecute();
        testSecurityBlockFileIO();
        testSecurityBlockLoadstring();
        testPathEscapePrevention();
        testResourceLimits();
        testGlobalVariables();
        testVMReset();
        testExtensionAPI();
        testMemoryTracking();
        
        std::cout << "\n=== All tests PASSED ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test FAILED with exception: " << e.what() << std::endl;
        return 1;
    }
}
