add_library(dsp_nodes STATIC
    GpuFFTNode.cpp
    ConvolutionNode.cpp
)

target_include_directories(dsp_nodes PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dsp_nodes PRIVATE
    $<TARGET_NAME_IF_EXISTS:dsp_offload>
)

if(ENABLE_GPU)
    target_compile_definitions(dsp_nodes PRIVATE ENABLE_GPU)
endif()
