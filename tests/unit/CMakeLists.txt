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

    # Pattern CRDT Test
    add_executable(TestPatternCRDT TestPatternCRDT.cpp)
    target_include_directories(TestPatternCRDT PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(TestPatternCRDT PRIVATE GTest::gtest GTest::gtest_main)
    cppmusic_add_warning_flags(TestPatternCRDT)
    cppmusic_add_sanitizer_flags(TestPatternCRDT)
    add_test(NAME PatternCRDTTest COMMAND TestPatternCRDT)

    # Performance Advisor Test
    add_executable(TestPerformanceAdvisor TestPerformanceAdvisor.cpp)
    target_include_directories(TestPerformanceAdvisor PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(TestPerformanceAdvisor PRIVATE GTest::gtest GTest::gtest_main)
    cppmusic_add_warning_flags(TestPerformanceAdvisor)
    cppmusic_add_sanitizer_flags(TestPerformanceAdvisor)
    add_test(NAME PerformanceAdvisorTest COMMAND TestPerformanceAdvisor)

    # Integrity Hasher Test
    add_executable(TestIntegrityHasher TestIntegrityHasher.cpp)
    target_include_directories(TestIntegrityHasher PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(TestIntegrityHasher PRIVATE GTest::gtest GTest::gtest_main)
    cppmusic_add_warning_flags(TestIntegrityHasher)
    cppmusic_add_sanitizer_flags(TestIntegrityHasher)
    add_test(NAME IntegrityHasherTest COMMAND TestIntegrityHasher)

    # Undo Determinism Test
    add_executable(TestUndoDeterminism TestUndoDeterminism.cpp)
    target_include_directories(TestUndoDeterminism PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(TestUndoDeterminism PRIVATE GTest::gtest GTest::gtest_main)
    cppmusic_add_warning_flags(TestUndoDeterminism)
    cppmusic_add_sanitizer_flags(TestUndoDeterminism)
    add_test(NAME UndoDeterminismTest COMMAND TestUndoDeterminism)

    # Arrangement Metrics Test
    add_executable(TestArrangementMetrics TestArrangementMetrics.cpp)
    target_include_directories(TestArrangementMetrics PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(TestArrangementMetrics PRIVATE GTest::gtest GTest::gtest_main)
    cppmusic_add_warning_flags(TestArrangementMetrics)
    cppmusic_add_sanitizer_flags(TestArrangementMetrics)
    add_test(NAME ArrangementMetricsTest COMMAND TestArrangementMetrics)

else()
    message(STATUS "Google Test not found - unit tests will be built without GTest")
    
    # Pattern unit test (original - doesn't require GTest)
    add_executable(TestPattern
        TestPattern.cpp
    )
    target_include_directories(TestPattern PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(TestPattern PRIVATE model_layer)
    cppmusic_add_warning_flags(TestPattern)
    cppmusic_add_sanitizer_flags(TestPattern)
    add_test(NAME PatternUnitTest COMMAND TestPattern)
endif()
