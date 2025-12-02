# src/model/CMakeLists.txt
# Model layer: Pattern, DemoProjects

add_library(model_layer STATIC
    Pattern.cpp
    DemoProjects.cpp
)

target_include_directories(model_layer PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Link to project module for ProjectModel access (brings in JUCE dependencies)
target_link_libraries(model_layer PUBLIC
    project_model
)

# Set position-independent code for static library
set_target_properties(model_layer PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Asset database
add_subdirectory(assets)
