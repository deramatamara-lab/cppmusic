# Main application executable

set(MAIN_SOURCES
    main.cpp
)

set(MAIN_HEADERS
)

# Create executable
add_executable(DAWProject
    ${MAIN_SOURCES}
    ${MAIN_HEADERS}
)

target_include_directories(DAWProject PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link all our libraries
target_link_libraries(DAWProject PRIVATE
    ui_app
    ui_views
    ui_components
    ui_lookandfeel
    project_model
    audio_engine
    audio_dsp
    core_state
    core_utilities
    audio_processors
    audio_effects
    audio_synthesis
    ai_inference
    ai_models
)

# Link JUCE modules
# JUCE should be available via add_subdirectory in root CMakeLists.txt
if(TARGET juce::juce_core)
    target_link_libraries(DAWProject PRIVATE
        juce::juce_core
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
    )
else()
    message(FATAL_ERROR "JUCE is required to build the application. Please set JUCE_DIR or add JUCE as a submodule.")
endif()

# Platform-specific settings
if(APPLE)
    set_target_properties(DAWProject PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    )
endif()

if(WIN32)
    # Windows-specific settings
    set_target_properties(DAWProject PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

