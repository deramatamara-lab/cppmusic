# Performance tests

# Check if ImGui UI targets exist
if(TARGET cppmusic_godmode_ui)
    # UI Pipeline benchmark
    add_executable(benchmark_ui_pipeline
        BenchmarkUIPipeline.cpp
    )
    target_include_directories(benchmark_ui_pipeline PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(benchmark_ui_pipeline PRIVATE
        cppmusic_godmode_ui
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(benchmark_ui_pipeline PRIVATE 
            -Wno-conversion 
            -Wno-sign-conversion
        )
    endif()
    
    # Note: Benchmark is not added to ctest by default as it's slow
    # Run manually with: ./build/tests/perf/benchmark_ui_pipeline
endif()
