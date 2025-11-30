# AI Inference

set(AI_INFERENCE_SOURCES
    InferenceEngine.cpp
    AIProcessor.cpp
)

set(AI_INFERENCE_HEADERS
    InferenceEngine.h
    AIProcessor.h
)

add_library(ai_inference STATIC
    ${AI_INFERENCE_SOURCES}
    ${AI_INFERENCE_HEADERS}
)

target_include_directories(ai_inference PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ai_inference PUBLIC
    ai_models
    core_utilities
    ai_config
)

