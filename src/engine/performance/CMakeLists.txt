# Performance module CMakeLists.txt

add_library(engine_performance STATIC
    PerformanceAdvisor.cpp
)

target_include_directories(engine_performance PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(engine_performance)
cppmusic_add_sanitizer_flags(engine_performance)

# Set position-independent code for static library
set_target_properties(engine_performance PROPERTIES POSITION_INDEPENDENT_CODE ON)
