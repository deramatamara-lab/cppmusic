# AI Config module

set(AI_CONFIG_SOURCES
    AIConfig.cpp
)

set(AI_CONFIG_HEADERS
    AIConfig.h
)

add_library(ai_config STATIC
    ${AI_CONFIG_SOURCES}
    ${AI_CONFIG_HEADERS}
)

target_include_directories(ai_config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ai_config PUBLIC
    juce::juce_core
    juce::juce_data_structures
)
