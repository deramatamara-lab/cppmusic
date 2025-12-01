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

# Scheduler unit test
add_executable(TestScheduler
    TestScheduler.cpp
)

target_include_directories(TestScheduler PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(TestScheduler PRIVATE
    engine_skeleton
)

cppmusic_add_warning_flags(TestScheduler)
cppmusic_add_sanitizer_flags(TestScheduler)

add_test(NAME SchedulerUnitTest COMMAND TestScheduler)

# Pattern Probability/Conditions unit test
add_executable(TestPatternProbability
    TestPatternProbability.cpp
)

target_include_directories(TestPatternProbability PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(TestPatternProbability PRIVATE
    model_layer
)

cppmusic_add_warning_flags(TestPatternProbability)
cppmusic_add_sanitizer_flags(TestPatternProbability)

add_test(NAME PatternProbabilityUnitTest COMMAND TestPatternProbability)
