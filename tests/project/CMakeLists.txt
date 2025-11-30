# Project model tests

find_package(CURL REQUIRED)

add_executable(project_tests
    main.cpp
    ProjectModelTest.cpp
    PatternTest.cpp
    UndoManagerTest.cpp
    PatternJsonParserTest.cpp
)

target_include_directories(project_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(project_tests PRIVATE
    project_model
    CURL::libcurl
)

if(TARGET juce::juce_core)
    target_link_libraries(project_tests PRIVATE
        juce::juce_core
        juce::juce_graphics
    )
endif()

add_test(NAME ProjectTests COMMAND project_tests)

