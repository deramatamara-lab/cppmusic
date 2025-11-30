# Project Model

set(PROJECT_SOURCES
    Track.cpp
    Clip.cpp
    ClipContainer.cpp
    ProjectModel.cpp
    SelectionModel.cpp
    AutomationCurve.cpp
    Pattern.cpp
    ProjectSerializer.cpp
    ProjectMigrator.cpp
    UndoManager.cpp
    commands/MoveClipCommand.cpp
    commands/TrimClipCommand.cpp
    commands/CreateClipCommand.cpp
    commands/GroupClipsCommand.cpp
    commands/MoveContainerCommand.cpp
    commands/UpdatePatternNotesCommand.cpp
    PatternJsonParser.cpp
)

set(PROJECT_HEADERS
    Track.h
    Clip.h
    ClipContainer.h
    ProjectModel.h
    SelectionModel.h
    AutomationCurve.h
    Pattern.h
    ProjectSerializer.h
    ProjectMigrator.h
    UndoManager.h
    commands/MoveClipCommand.h
    commands/TrimClipCommand.h
    commands/CreateClipCommand.h
    commands/GroupClipsCommand.h
    commands/MoveContainerCommand.h
    commands/UpdatePatternNotesCommand.h
    PatternJsonParser.h
)

add_library(project_model STATIC
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
)

target_include_directories(project_model PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(TARGET juce::juce_core)
    target_link_libraries(project_model PUBLIC
        juce::juce_graphics
        juce::juce_core
    )
endif()

