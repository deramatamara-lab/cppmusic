# Sandbox platform CMakeLists.txt

add_library(platform_sandbox STATIC
    SandboxRunner.cpp
)

target_include_directories(platform_sandbox PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(platform_sandbox)
cppmusic_add_sanitizer_flags(platform_sandbox)

# Set position-independent code for static library
set_target_properties(platform_sandbox PROPERTIES POSITION_INDEPENDENT_CODE ON)
