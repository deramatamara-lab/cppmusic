# AI tests

find_package(CURL)

add_executable(ai_tests
    main.cpp
    InferenceEngineTest.cpp
    ../../src/ai/config/AIConfig.cpp
)

target_link_libraries(ai_tests PRIVATE
    ai_inference
)

if(CURL_FOUND)
    target_link_libraries(ai_tests PRIVATE ${CURL_LIBRARIES})
    target_include_directories(ai_tests PRIVATE ${CURL_INCLUDE_DIRS})
endif()

if(TARGET juce::juce_core)
    target_link_libraries(ai_tests PRIVATE
        juce::juce_core
        juce::juce_data_structures
    )
endif()

add_test(NAME AITests COMMAND ai_tests)

