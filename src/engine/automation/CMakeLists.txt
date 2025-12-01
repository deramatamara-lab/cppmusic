# Automation module CMakeLists.txt

add_library(engine_automation STATIC
    AutomationClip.cpp
    AutomationVersionStore.cpp
)

target_include_directories(engine_automation PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(engine_automation)
cppmusic_add_sanitizer_flags(engine_automation)

# Set position-independent code for static library
set_target_properties(engine_automation PROPERTIES POSITION_INDEPENDENT_CODE ON)
