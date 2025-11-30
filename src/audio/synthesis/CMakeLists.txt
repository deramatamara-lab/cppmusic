# Audio Synthesis

set(AUDIO_SYNTHESIS_SOURCES
    Synthesizer.cpp
    Oscillator.cpp
)

set(AUDIO_SYNTHESIS_HEADERS
    Synthesizer.h
    Oscillator.h
)

add_library(audio_synthesis STATIC
    ${AUDIO_SYNTHESIS_SOURCES}
    ${AUDIO_SYNTHESIS_HEADERS}
)

target_include_directories(audio_synthesis PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(audio_synthesis PUBLIC
    audio_processors
)

# Link JUCE modules if available
if(TARGET juce::juce_core)
    target_link_libraries(audio_synthesis PUBLIC
        juce::juce_audio_basics
        juce::juce_core
    )
endif()
