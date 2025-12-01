# tests/perf/CMakeLists.txt
# Performance benchmarks

# GPU Convolution Benchmark
add_executable(BenchGpuConvolution BenchGpuConvolution.cpp)
target_include_directories(BenchGpuConvolution PRIVATE ${CMAKE_SOURCE_DIR}/src)
cppmusic_add_warning_flags(BenchGpuConvolution)

if(ENABLE_GPU)
    target_compile_definitions(BenchGpuConvolution PRIVATE ENABLE_GPU)
endif()

# Parameter Propagation Benchmark
add_executable(BenchParamPropagation BenchParamPropagation.cpp)
target_include_directories(BenchParamPropagation PRIVATE ${CMAKE_SOURCE_DIR}/src)
cppmusic_add_warning_flags(BenchParamPropagation)
