# AI Models

set(AI_MODELS_SOURCES
    AIModelManager.cpp
    AIModel.cpp
    GrooveExtractor.cpp
    MelodyGenerator.cpp
    ChordAdvisor.cpp
)

set(AI_MODELS_HEADERS
    AIModelManager.h
    AIModel.h
    GrooveExtractor.h
    MelodyGenerator.h
    ChordAdvisor.h
)

add_library(ai_models STATIC
    ${AI_MODELS_SOURCES}
    ${AI_MODELS_HEADERS}
)

target_include_directories(ai_models PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ai_models PUBLIC
    core_utilities
    ai_inference
    project_model
)

if(TARGET juce::juce_core)
    target_link_libraries(ai_models PUBLIC
        juce::juce_core
    )
endif()

