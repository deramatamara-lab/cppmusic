# Project Model

set(PROJECT_SOURCES
    Track.cpp
    Clip.cpp
    ProjectModel.cpp
    SelectionModel.cpp
    AutomationCurve.cpp
    Pattern.cpp
    ProjectSerializer.cpp
    ProjectMigrator.cpp
)

set(PROJECT_HEADERS
    Track.h
    Clip.h
    ProjectModel.h
    SelectionModel.h
    AutomationCurve.h
    Pattern.h
    ProjectSerializer.h
    ProjectMigrator.h
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

