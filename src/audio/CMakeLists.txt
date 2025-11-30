# Audio processing module

# Core audio library with all DSP components
add_library(cppmusic_audio STATIC
    # Core DSP processors
    AdvancedSynthesizer_rt.hpp  # Single-header synthesizer implementation
    AnalogModeledEQ.h  # Header-only implementation
    AuroraReverb.cpp
    AuroraReverb.h
    SpectralAnalysisEngine_and_MasterAudioProcessor.cpp
    SpectralAnalysisEngine.h
    MasterAudioProcessor.h

    # Legacy components (if needed)
    # Add other audio files here
)

# Link with JUCE audio modules
target_link_libraries(cppmusic_audio
    PUBLIC
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_core
    PRIVATE
        cppmusic_core  # For EngineContext and RTMemoryPool
)

# Include directories
target_include_directories(cppmusic_audio
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compile definitions for development rules
target_compile_definitions(cppmusic_audio
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="CPPMusic DAW"
        JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
    PRIVATE
        JUCE_UNIT_TESTS=0  # Disable JUCE unit tests to avoid third-party warnings-as-errors
)

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # target_compile_options(cppmusic_audio PRIVATE
    #     -ffast-math
    #     -march=native
    #     -mtune=native
    # )
endif()

# Subdirectories for organized components
add_subdirectory(processors)
add_subdirectory(effects)
add_subdirectory(synthesis)
add_subdirectory(engine)
add_subdirectory(dsp)

