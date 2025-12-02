# UI Performance module - Performance monitoring and profiling

add_library(ui_performance STATIC
    UIPerformance.hpp
    UIPerformance.cpp
)

target_include_directories(ui_performance PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(TARGET juce::juce_core)
    target_link_libraries(ui_performance PUBLIC
        juce::juce_core
    )
endif()
