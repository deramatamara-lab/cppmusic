# UI Components

set(UI_COMPONENTS_SOURCES
    WaveformViewer.cpp
    ControlPanel.cpp
    FlagshipDevicePanel.cpp
    PatternSequencerPanel.cpp
    SessionLauncherView.cpp
    ClipLauncherSystem.cpp
    StepSequencer.cpp
    Panel.cpp
    Dial.cpp
    Meter.cpp
)

set(UI_COMPONENTS_HEADERS
    WaveformViewer.h
    ControlPanel.h
    FlagshipDevicePanel.h
    PatternSequencerPanel.h
    SessionLauncherView.h
    ClipLauncherSystem.h
    StepSequencer.h
    Panel.h
    Dial.h
    Meter.h
)

add_library(ui_components STATIC
    ${UI_COMPONENTS_SOURCES}
    ${UI_COMPONENTS_HEADERS}
)

target_include_directories(ui_components PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui_components PUBLIC
    ui_lookandfeel
)

# Link JUCE modules
if(TARGET juce::juce_core)
    target_link_libraries(ui_components PUBLIC 
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_dsp
    )
endif()

