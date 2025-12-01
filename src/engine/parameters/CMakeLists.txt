# Parameters module CMakeLists.txt

add_library(engine_parameters STATIC
    ParamSignal.cpp
    ParamRegistry.cpp
    ModMatrix.cpp
)

target_include_directories(engine_parameters PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(engine_parameters)
cppmusic_add_sanitizer_flags(engine_parameters)

# Set position-independent code for static library
set_target_properties(engine_parameters PROPERTIES POSITION_INDEPENDENT_CODE ON)
