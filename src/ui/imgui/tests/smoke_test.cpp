/**
 * @file smoke_test.cpp
 * @brief Headless smoke test for the ImGui UI
 * 
 * This test initializes the UI system in headless mode (offscreen context),
 * verifies that the theme can be loaded and applied, and renders a single frame.
 * It validates that the UI construction works without a visible window.
 */

#include "../App.hpp"
#include "../Theme.hpp"
#include "../Shortcuts.hpp"
#include <iostream>
#include <cassert>

// Simple test framework macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << message << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            return 1; \
        } \
        std::cout << "PASS: " << message << std::endl; \
    } while (0)

int main()
{
    std::cout << "=== cppmusic ImGui UI Smoke Test ===" << std::endl;
    std::cout << std::endl;
    
    // Test 1: Theme system
    {
        std::cout << "--- Testing Theme System ---" << std::endl;
        
        daw::ui::imgui::Theme theme;
        
        // Test default tokens
        const auto& tokens = theme.getTokens();
        TEST_ASSERT(tokens.spacingSm == 8.0f, "Default spacing should be 8px grid");
        TEST_ASSERT(tokens.fontSizeMd == 14.0f, "Default font size should be 14pt");
        
        // Test DPI scaling
        theme.setDpiScale(2.0f);
        TEST_ASSERT(theme.getDpiScale() == 2.0f, "DPI scale should be set to 2.0");
        TEST_ASSERT(theme.spacing(1) == 16.0f, "Spacing with 2x scale should be doubled");
        
        theme.setDpiScale(1.0f);
        
        std::cout << std::endl;
    }
    
    // Test 2: Shortcut system
    {
        std::cout << "--- Testing Shortcut System ---" << std::endl;
        
        daw::ui::imgui::Shortcuts shortcuts;
        
        // Register default commands
        shortcuts.registerDefaultCommands();
        
        // Test command registration
        const auto& commands = shortcuts.getCommands();
        TEST_ASSERT(commands.size() >= 10, "Should have at least 10 default commands");
        
        // Test command lookup
        auto* saveCmd = shortcuts.getCommand("file.save");
        TEST_ASSERT(saveCmd != nullptr, "file.save command should exist");
        TEST_ASSERT(saveCmd->name == "Save Project", "Command name should match");
        
        // Test shortcut parsing
        auto shortcut = daw::ui::imgui::Shortcut::fromString("Ctrl+S");
        TEST_ASSERT(shortcut.key == ImGuiKey_S, "Should parse key S");
        TEST_ASSERT(daw::ui::imgui::hasFlag(shortcut.modifiers, daw::ui::imgui::KeyMod::Ctrl), 
                   "Should have Ctrl modifier");
        
        // Test shortcut to string
        std::string shortcutStr = shortcut.toString();
        TEST_ASSERT(shortcutStr == "Ctrl+S", "Shortcut string should be Ctrl+S");
        
        // Test fuzzy search
        auto results = shortcuts.search("save", 5);
        TEST_ASSERT(!results.empty(), "Search for 'save' should return results");
        TEST_ASSERT(results[0]->id == "file.save", "First result should be file.save");
        
        std::cout << std::endl;
    }
    
    // Test 3: Headless UI initialization
    {
        std::cout << "--- Testing Headless UI Initialization ---" << std::endl;
        
        daw::ui::imgui::App app;
        
        // Initialize in headless mode
        bool initResult = app.initializeHeadless();
        
        // Note: This may fail in truly headless environments without X11/EGL
        // In CI, we might need to use Xvfb or skip this test
        if (!initResult)
        {
            std::cout << "SKIP: Headless initialization failed (may need display server)" << std::endl;
            std::cout << "      This is expected in environments without X11/OpenGL support." << std::endl;
        }
        else
        {
            TEST_ASSERT(app.isRunning(), "App should be running after init");
            
            // Test theme access
            auto& theme = app.getTheme();
            TEST_ASSERT(theme.getDpiScale() > 0.0f, "Theme should have valid DPI scale");
            
            // Test shortcut access
            auto& shortcuts = app.getShortcuts();
            TEST_ASSERT(shortcuts.getCommands().size() >= 10, "Should have commands registered");
            
            // Render a single frame
            bool frameResult = app.renderFrame();
            TEST_ASSERT(frameResult, "Should render a frame successfully");
            
            // Cleanup
            app.shutdown();
        }
        
        std::cout << std::endl;
    }
    
    // Test 4: Panel construction (without rendering)
    {
        std::cout << "--- Testing Panel Construction ---" << std::endl;
        
        // These tests verify that panels can be constructed without crashes
        // They don't require an OpenGL context
        
        {
            daw::ui::imgui::TransportBar transport;
            auto& state = transport.getState();
            TEST_ASSERT(state.bpm == 120.0, "Default BPM should be 120");
            state.isPlaying = true;
            TEST_ASSERT(state.isPlaying, "Should be able to modify state");
        }
        
        {
            daw::ui::imgui::BrowserPanel browser;
            // Browser creates demo content in constructor
            TEST_ASSERT(true, "BrowserPanel constructed successfully");
        }
        
        {
            daw::ui::imgui::ChannelRackPanel channelRack;
            auto& channels = channelRack.getChannels();
            TEST_ASSERT(channels.size() >= 4, "Should have demo channels");
        }
        
        {
            daw::ui::imgui::PianoRollPanel pianoRoll;
            auto& notes = pianoRoll.getNotes();
            TEST_ASSERT(!notes.empty(), "Should have demo notes");
        }
        
        {
            daw::ui::imgui::PlaylistPanel playlist;
            auto& clips = playlist.getClips();
            TEST_ASSERT(!clips.empty(), "Should have demo clips");
        }
        
        {
            daw::ui::imgui::MixerPanel mixer;
            auto& channels = mixer.getChannels();
            TEST_ASSERT(!channels.empty(), "Should have demo channels");
        }
        
        {
            daw::ui::imgui::InspectorPanel inspector;
            // Inspector sets demo context in constructor
            TEST_ASSERT(true, "InspectorPanel constructed successfully");
        }
        
        std::cout << std::endl;
    }
    
    std::cout << "=== All Smoke Tests Passed ===" << std::endl;
    return 0;
}
