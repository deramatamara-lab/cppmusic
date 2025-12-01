# Tagging AI CMakeLists.txt

add_library(ai_tagging STATIC
    FeatureExtractor.cpp
    TagClassifier.cpp
)

target_include_directories(ai_tagging PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(ai_tagging)
cppmusic_add_sanitizer_flags(ai_tagging)

# Set position-independent code for static library
set_target_properties(ai_tagging PROPERTIES POSITION_INDEPENDENT_CODE ON)
