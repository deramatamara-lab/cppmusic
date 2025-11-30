# Audio Engine

set(AUDIO_ENGINE_SOURCES
    DawEngine.cpp
    Transport.cpp
    AudioGraph.cpp
    EngineContext.cpp
    ModulationMatrix.cpp
    PatternPlayer.cpp
    PatternPlaybackEngine.cpp
)

set(AUDIO_ENGINE_HEADERS
    DawEngine.h
    Transport.h
    AudioGraph.h
    EngineContext.h
    BlockSizeOptimizer.h
    ModulationMatrix.h
    ParameterSmoother.h
    PatternPlayer.h
    PatternPlaybackEngine.h
)

add_library(audio_engine STATIC
    ${AUDIO_ENGINE_SOURCES}
    ${AUDIO_ENGINE_HEADERS}
)

target_include_directories(audio_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(audio_engine PUBLIC
    audio_dsp
    core_utilities
)

if(TARGET juce::juce_core)
    target_link_libraries(audio_engine PUBLIC
        juce::juce_audio_processors
        juce::juce_audio_devices
        juce::juce_audio_utils
        juce::juce_core
    )

    # Disable web browser component to avoid GTK/WebKit dependencies in tests
    target_compile_definitions(audio_engine PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )
endif()
