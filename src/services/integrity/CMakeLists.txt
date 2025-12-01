# Integrity services CMakeLists.txt

add_library(services_integrity STATIC
    StateHasher.cpp
)

target_include_directories(services_integrity PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(services_integrity)
cppmusic_add_sanitizer_flags(services_integrity)

# Set position-independent code for static library
set_target_properties(services_integrity PROPERTIES POSITION_INDEPENDENT_CODE ON)
