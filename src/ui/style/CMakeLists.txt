# UI Style module - Look and Feel / Design System

add_library(ui_style STATIC
    LookAndFeel.hpp
    LookAndFeel.cpp
    IconManager.hpp
    IconManager.cpp
    UIScaleManager.hpp
    UIScaleManager.cpp
)

target_include_directories(ui_style PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(TARGET juce::juce_gui_basics)
    target_link_libraries(ui_style PUBLIC
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_core
    )
endif()
