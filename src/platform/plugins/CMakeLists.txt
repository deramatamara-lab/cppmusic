# Plugins platform CMakeLists.txt

add_library(platform_plugins STATIC
    PluginInspector.cpp
)

target_include_directories(platform_plugins PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(platform_plugins PUBLIC
    platform_sandbox
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(platform_plugins)
cppmusic_add_sanitizer_flags(platform_plugins)

# Set position-independent code for static library
set_target_properties(platform_plugins PROPERTIES POSITION_INDEPENDENT_CODE ON)
