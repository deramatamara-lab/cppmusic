# Undo services CMakeLists.txt

add_library(services_undo STATIC
    UndoService.cpp
)

target_include_directories(services_undo PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(services_undo)
cppmusic_add_sanitizer_flags(services_undo)

# Set position-independent code for static library
set_target_properties(services_undo PROPERTIES POSITION_INDEPENDENT_CODE ON)
