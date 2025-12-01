# src/engine/CMakeLists.txt
# Engine skeleton: AudioGraph, Transport, and Scheduler

add_library(engine_skeleton STATIC
    AudioGraph.cpp
    Transport.cpp
    Scheduler.cpp
)

target_include_directories(engine_skeleton PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(engine_skeleton)
cppmusic_add_sanitizer_flags(engine_skeleton)

# Set position-independent code for static library
set_target_properties(engine_skeleton PROPERTIES POSITION_INDEPENDENT_CODE ON)
