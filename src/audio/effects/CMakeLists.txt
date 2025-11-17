# Audio Effects

set(AUDIO_EFFECTS_SOURCES
    Compressor.cpp
    Reverb.cpp
    Delay.cpp
)

set(AUDIO_EFFECTS_HEADERS
    Compressor.h
    Reverb.h
    Delay.h
)

add_library(audio_effects STATIC
    ${AUDIO_EFFECTS_SOURCES}
    ${AUDIO_EFFECTS_HEADERS}
)

target_include_directories(audio_effects PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(audio_effects PUBLIC
    audio_processors
)

# Link JUCE if available
# target_link_libraries(audio_effects PUBLIC juce::juce_audio_basics)

