add_library(dsp_offload STATIC
    GpuContext.cpp
    OffloadManager.cpp
)

target_include_directories(dsp_offload PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Optional Vulkan dependency
if(ENABLE_GPU)
    find_package(Vulkan)
    if(Vulkan_FOUND)
        target_link_libraries(dsp_offload PRIVATE Vulkan::Vulkan)
        target_compile_definitions(dsp_offload PUBLIC ENABLE_GPU)
    else()
        message(WARNING "ENABLE_GPU requested but Vulkan not found. GPU features disabled.")
    endif()
endif()
