# Plugins

set(PLUGINS_SOURCES
    PluginHost.cpp
    PluginDatabase.cpp
)

set(PLUGINS_HEADERS
    PluginHost.h
    PluginDatabase.h
)

add_library(plugins STATIC
    ${PLUGINS_SOURCES}
    ${PLUGINS_HEADERS}
)

target_include_directories(plugins PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(plugins PUBLIC
    core_utilities
)

if(TARGET juce::juce_core)
    target_link_libraries(plugins PUBLIC
        juce::juce_audio_processors
        juce::juce_core
    )
endif()

