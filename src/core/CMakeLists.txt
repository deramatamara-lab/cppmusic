# Core module

add_subdirectory(utilities)
add_subdirectory(state)

set(CORE_SOURCES
    ServiceLocator.cpp
    EngineContext.cpp
    RTMemoryPool.cpp
)

set(CORE_HEADERS
    ServiceLocator.h
    EngineContext.h
    RTMemoryPool.h
    LockFreeRingBuffer.h
    RealtimeMessageQueue.h
    ServiceRegistry.h
)

add_library(cppmusic_core STATIC
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

target_include_directories(cppmusic_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cppmusic_core PUBLIC
    core_utilities
    core_state
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_events
)

