# tests/unit/CMakeLists.txt
# Unit tests for engine skeleton and model layer

# Pattern unit test
add_executable(TestPattern
    TestPattern.cpp
)

target_include_directories(TestPattern PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(TestPattern PRIVATE
    model_layer
)

# Apply flags
cppmusic_add_warning_flags(TestPattern)
cppmusic_add_sanitizer_flags(TestPattern)

# Register with CTest
add_test(NAME PatternUnitTest COMMAND TestPattern)
