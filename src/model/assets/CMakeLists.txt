# Assets model CMakeLists.txt

add_library(model_assets STATIC
    AssetDB.cpp
)

target_include_directories(model_assets PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(model_assets PUBLIC
    ai_tagging
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(model_assets)
cppmusic_add_sanitizer_flags(model_assets)

# Set position-independent code for static library
set_target_properties(model_assets PROPERTIES POSITION_INDEPENDENT_CODE ON)
