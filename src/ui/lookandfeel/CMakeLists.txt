# UI LookAndFeel

set(UI_LOOKANDFEEL_SOURCES
    CustomLookAndFeel.cpp
    DesignSystem.cpp
    DesignTokens.cpp
    MainLookAndFeel.cpp
    ThemeManager.cpp
    EnhancedMainLookAndFeel.cpp
    EnhancedThemeManager.cpp
)

set(UI_LOOKANDFEEL_HEADERS
    CustomLookAndFeel.h
    DesignSystem.h
    DesignTokens.h
    MainLookAndFeel.h
    ThemeManager.h
    EnhancedMainLookAndFeel.h
    EnhancedThemeManager.h
)

add_library(ui_lookandfeel STATIC
    ${UI_LOOKANDFEEL_SOURCES}
    ${UI_LOOKANDFEEL_HEADERS}
)

target_include_directories(ui_lookandfeel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui_lookandfeel PUBLIC
    ui_effects
    ui_animation
)

# Link JUCE modules
if(TARGET juce::juce_core)
    target_link_libraries(ui_lookandfeel PUBLIC
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_core
    )
endif()

