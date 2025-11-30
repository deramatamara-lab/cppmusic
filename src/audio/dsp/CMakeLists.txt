# Audio DSP

set(AUDIO_DSP_SOURCES
    TrackStrip.cpp
    LFO.cpp
    Envelope.cpp
)

set(AUDIO_DSP_HEADERS
    TrackStrip.h
    Modulator.h
    LFO.h
    Envelope.h
)

add_library(audio_dsp STATIC
    ${AUDIO_DSP_SOURCES}
    ${AUDIO_DSP_HEADERS}
)

target_include_directories(audio_dsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(audio_dsp PUBLIC
    juce::juce_core
    juce::juce_audio_basics
)

