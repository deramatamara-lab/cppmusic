# Integration module - connects audio processing with UI and core systems

add_library(cppmusic_integration STATIC
    # EQ Service integration
    EqualizerService.cpp
    EqualizerService.h
)

# Include directories
target_include_directories(cppmusic_integration
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(cppmusic_integration
    PUBLIC
        cppmusic_core
        cppmusic_audio
        cppmusic_ui
        juce::juce_audio_processors
        juce::juce_core
    PRIVATE
        juce::juce_events
)

# Compile definitions
target_compile_definitions(cppmusic_integration
    PRIVATE
        JUCE_UNIT_TESTS=1
        JUCE_USE_OGGVORBIS=0
        JUCE_USE_MP3AUDIOFORMAT=0
        JUCE_USE_LAME_AUDIO_FORMAT=0
        JUCE_USE_FLAC=0
)

# Export symbols for plugin builds
if(WIN32)
    target_compile_definitions(cppmusic_integration PRIVATE JUCE_DLL_BUILD=1)
endif()

# Optimization flags for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(cppmusic_integration PRIVATE
            -O3
            -ffast-math
            -DNDEBUG
        )
    elseif(MSVC)
        target_compile_options(cppmusic_integration PRIVATE
            /O2
            /fp:fast
            /DNDEBUG
        )
    endif()
endif()
