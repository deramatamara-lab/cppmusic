# UI Effects module - Visual effects and glassmorphism

set(UI_EFFECTS_SOURCES
    GlassMorphismRenderer.cpp
)

set(UI_EFFECTS_HEADERS
    GlassMorphismRenderer.h
    UltraSleekEffects.h
)

add_library(ui_effects STATIC
    ${UI_EFFECTS_SOURCES}
    ${UI_EFFECTS_HEADERS}
)

target_include_directories(ui_effects PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui_effects PUBLIC
    ui_lookandfeel
)

if(TARGET juce::juce_gui_basics)
    target_link_libraries(ui_effects PUBLIC
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_core
    )
endif()

