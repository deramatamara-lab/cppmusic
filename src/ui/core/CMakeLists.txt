# UI Core module - Enhanced base components

set(UI_CORE_SOURCES
    ModernUIComponent.cpp
    AnimationHelper.cpp
    AppState.cpp
    Commands.cpp
)

set(UI_CORE_HEADERS
    ModernUIComponent.h
    PhysicsAnimation.h
    AnimationHelper.h
    AppState.hpp
    Commands.hpp
)

add_library(ui_core STATIC
    ${UI_CORE_SOURCES}
    ${UI_CORE_HEADERS}
)

target_include_directories(ui_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui_core PUBLIC
    ui_lookandfeel
)

if(TARGET juce::juce_gui_basics)
    target_link_libraries(ui_core PUBLIC
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_core
    )
endif()

