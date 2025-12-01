# src/model/CMakeLists.txt
# Model layer: Pattern

add_library(model_layer STATIC
    Pattern.cpp
)

target_include_directories(model_layer PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(model_layer)
cppmusic_add_sanitizer_flags(model_layer)

# Set position-independent code for static library
set_target_properties(model_layer PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Asset database
add_subdirectory(assets)
