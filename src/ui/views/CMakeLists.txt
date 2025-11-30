# UI Views

set(UI_VIEWS_SOURCES
    MainView.cpp
    TransportBar.cpp
    BrowserPanel.cpp
    ArrangeView.cpp
    TrackHeaderComponent.cpp
    InspectorPanel.cpp
    MixerView.cpp
    MixerStrip.cpp
    PianoRollView.cpp
)

set(UI_VIEWS_HEADERS
    MainView.h
    TransportBar.h
    BrowserPanel.h
    ArrangeView.h
    TrackHeaderComponent.h
    InspectorPanel.h
    MixerView.h
    MixerStrip.h
    PianoRollView.h
)

add_library(ui_views STATIC
    ${UI_VIEWS_SOURCES}
    ${UI_VIEWS_HEADERS}
)

target_include_directories(ui_views PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui_views PUBLIC
    ui_lookandfeel
    project_model
    audio_engine
    ui_components
    plugins
    ai_inference
    ai_config
)

if(TARGET juce::juce_core)
    target_link_libraries(ui_views PUBLIC
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_core
        juce::juce_audio_devices
    )
endif()

