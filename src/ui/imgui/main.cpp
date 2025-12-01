/**
 * @file main.cpp
 * @brief Main entry point for the cppmusic ImGui UI application
 * 
 * This is a sample application demonstrating the ultra-polished
 * Dear ImGui-based DAW interface with SDL2 + OpenGL backend.
 */

#include "App.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[])
{
    // Parse command line arguments
    daw::ui::imgui::AppConfig config;
    
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        
        if (arg == "--fullscreen" || arg == "-f")
        {
            config.fullscreen = true;
        }
        else if (arg == "--no-vsync")
        {
            config.vsync = false;
        }
        else if ((arg == "--theme" || arg == "-t") && i + 1 < argc)
        {
            config.themePath = argv[++i];
        }
        else if ((arg == "--width" || arg == "-w") && i + 1 < argc)
        {
            config.windowWidth = std::atoi(argv[++i]);
        }
        else if ((arg == "--height" || arg == "-h") && i + 1 < argc)
        {
            config.windowHeight = std::atoi(argv[++i]);
        }
        else if (arg == "--help")
        {
            std::cout << "cppmusic DAW - ImGui UI Demo\n"
                      << "\n"
                      << "Usage: cppmusic_imgui_app [options]\n"
                      << "\n"
                      << "Options:\n"
                      << "  -f, --fullscreen    Start in fullscreen mode\n"
                      << "  --no-vsync          Disable vertical sync\n"
                      << "  -t, --theme <path>  Path to theme JSON file\n"
                      << "  -w, --width <px>    Window width (default: 1920)\n"
                      << "  -h, --height <px>   Window height (default: 1080)\n"
                      << "  --help              Show this help message\n"
                      << "\n"
                      << "Keyboard Shortcuts:\n"
                      << "  Ctrl+K              Open command palette\n"
                      << "  F12                 Toggle performance overlay\n"
                      << "  Space               Play/Pause\n"
                      << "  Ctrl+S              Save project\n"
                      << "  Ctrl+Z              Undo\n"
                      << "  Ctrl+Y              Redo\n"
                      << std::endl;
            return 0;
        }
    }
    
    // Create and initialize application
    daw::ui::imgui::App app;
    
    if (!app.initialize(config))
    {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }
    
    std::cout << "cppmusic DAW - ImGui UI Demo\n"
              << "Press Ctrl+K for command palette, F12 for performance overlay\n"
              << std::endl;
    
    // Run main loop
    int exitCode = app.run();
    
    return exitCode;
}
