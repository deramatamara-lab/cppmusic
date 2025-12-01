# Arrangement AI CMakeLists.txt

add_library(ai_arrangement STATIC
    ArrangementAnalyzer.cpp
    SuggestionEngine.cpp
)

target_include_directories(ai_arrangement PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(ai_arrangement)
cppmusic_add_sanitizer_flags(ai_arrangement)

# Set position-independent code for static library
set_target_properties(ai_arrangement PROPERTIES POSITION_INDEPENDENT_CODE ON)
