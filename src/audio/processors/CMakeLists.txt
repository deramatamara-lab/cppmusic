# Audio Processors

set(AUDIO_PROCESSORS_SOURCES
    AudioProcessorBase.cpp
)

set(AUDIO_PROCESSORS_HEADERS
    AudioProcessorBase.h
)

add_library(audio_processors STATIC
    ${AUDIO_PROCESSORS_SOURCES}
    ${AUDIO_PROCESSORS_HEADERS}
)

target_include_directories(audio_processors PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link JUCE if available
# target_link_libraries(audio_processors PUBLIC juce::juce_audio_basics)

