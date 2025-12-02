# tests/unit/CMakeLists.txt
# Unit tests for engine skeleton and model layer

# Find Google Test
find_package(GTest QUIET)

if(GTest_FOUND)
    message(STATUS "Google Test found - building unit tests")

    # Pattern unit test (original)
    add_executable(TestPattern
        TestPattern.cpp
    )
    target_include_directories(TestPattern PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(TestPattern PRIVATE model_layer)
    cppmusic_add_warning_flags(TestPattern)
    cppmusic_add_sanitizer_flags(TestPattern)
    add_test(NAME PatternUnitTest COMMAND TestPattern)

    # Parameter Graph Test
    add_executable(TestParamGraph TestParamGraph.cpp)
    target_include_directories(TestParamGraph PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(TestParamGraph PRIVATE GTest::gtest GTest::gtest_main)
    cppmusic_add_warning_flags(TestParamGraph)
    cppmusic_add_sanitizer_flags(TestParamGraph)
    add_test(NAME ParamGraphTest COMMAND TestParamGraph)

    # Automation Layers Test
    add_executable(TestAutomationLayers TestAutomationLayers.cpp)
    target_include_directories(TestAutomationLayers PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(TestAutomationLayers PRIVATE GTest::gtest GTest::gtest_main)
    cppmusic_add_warning_flags(TestAutomationLayers)
    cppmusic_add_sanitizer_flags(TestAutomationLayers)
    add_test(NAME AutomationLayersTest COMMAND TestAutomationLayers)
endif()
