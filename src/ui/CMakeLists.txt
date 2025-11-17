# UI module

add_subdirectory(components)
add_subdirectory(lookandfeel)
add_subdirectory(views)

# App and MainWindow
set(UI_APP_SOURCES
    App.cpp
    MainWindow.cpp
)

set(UI_APP_HEADERS
    App.h
    MainWindow.h
)

add_library(ui_app STATIC
    ${UI_APP_SOURCES}
    ${UI_APP_HEADERS}
)

target_include_directories(ui_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui_app PUBLIC
    ui_views
    ui_lookandfeel
)

if(TARGET juce::juce_core)
    target_link_libraries(ui_app PUBLIC
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_core
        juce::juce_audio_devices
    )
endif()

