# Core Utilities

set(CORE_UTILITIES_SOURCES
    Logger.cpp
    MathUtils.cpp
    JobSystem.cpp
)

set(CORE_UTILITIES_HEADERS
    Logger.h
    MathUtils.h
    LockFreeQueue.h
    PerformanceMonitor.h
    JobSystem.h
)

add_library(core_utilities STATIC
    ${CORE_UTILITIES_SOURCES}
    ${CORE_UTILITIES_HEADERS}
)

target_include_directories(core_utilities PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core_utilities PUBLIC
    juce::juce_core
)

