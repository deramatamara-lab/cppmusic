# Harmony analysis CMakeLists.txt

add_library(ai_harmony STATIC
    HarmonicAnalyzer.cpp
)

target_include_directories(ai_harmony PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(ai_harmony)
cppmusic_add_sanitizer_flags(ai_harmony)

# Set position-independent code for static library
set_target_properties(ai_harmony PROPERTIES POSITION_INDEPENDENT_CODE ON)
