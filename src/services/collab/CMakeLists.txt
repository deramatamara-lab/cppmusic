# Collaboration services CMakeLists.txt

add_library(services_collab STATIC
    PatternCRDT.cpp
    SessionServer.cpp
)

target_include_directories(services_collab PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(services_collab PUBLIC
    model_layer
)

# Apply warning and security flags from ToolchainFlags.cmake
cppmusic_add_warning_flags(services_collab)
cppmusic_add_sanitizer_flags(services_collab)

# Set position-independent code for static library
set_target_properties(services_collab PROPERTIES POSITION_INDEPENDENT_CODE ON)
