# UI module

# Core UI library with professional components
add_library(cppmusic_ui STATIC
    # EQ Editor and professional components
    AnalogEQEditor.cpp
    AnalogEQEditor.h
    AuroraReverbEditor.cpp
    AuroraReverbEditor.h

    # Main DAW Views
    views/DAWMainViews.cpp
    views/DAWMainViews.hpp

    # Component Library Showcase
    showcase/ComponentLibrary.cpp
    showcase/ComponentLibrary.hpp
)

# Add optional files if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/App.cpp")
    target_sources(cppmusic_ui PRIVATE App.cpp)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.cpp")
    target_sources(cppmusic_ui PRIVATE MainWindow.cpp)
endif()

# Include directories
target_include_directories(cppmusic_ui
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link with JUCE GUI modules
target_link_libraries(cppmusic_ui
    PUBLIC
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_core
        juce::juce_audio_processors
        juce::juce_audio_devices
        cppmusic_audio  # For EQ integration
        cppmusic_core   # For EngineContext
        ui_animation    # For AdaptiveAnimationService
)

# Compile definitions
target_compile_definitions(cppmusic_ui
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# Subdirectories for organized components (if they exist)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/core/CMakeLists.txt")
    add_subdirectory(core)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/effects/CMakeLists.txt")
    add_subdirectory(effects)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/animation/CMakeLists.txt")
    add_subdirectory(animation)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/components/CMakeLists.txt")
    add_subdirectory(components)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lookandfeel/CMakeLists.txt")
    add_subdirectory(lookandfeel)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/views/CMakeLists.txt")
    add_subdirectory(views)
endif()

# Legacy compatibility alias
add_library(ui_app ALIAS cppmusic_ui)

# ImGui-based UI module (SDL2 + OpenGL)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/imgui/CMakeLists.txt")
    add_subdirectory(imgui)
endif()

