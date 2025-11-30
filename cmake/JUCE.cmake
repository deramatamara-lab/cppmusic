# JUCE CMake integration helper
# This file provides helper functions for integrating JUCE into the build system

# Example function to find or add JUCE
function(setup_juce)
    if(TARGET juce::juce_core)
        return()
    endif()

    # Option 1: JUCE checked out at repository root
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" PARENT_SCOPE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
            add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
            return()
        else()
            message(WARNING "JUCE directory found but CMakeLists.txt not present; falling back to FetchContent")
        endif()
    # Option 2: JUCE cloned into external/JUCE (no git submodule required)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE")
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE" PARENT_SCOPE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
            add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE")
            return()
        else()
            message(WARNING "external/JUCE directory found but CMakeLists.txt not present; falling back to FetchContent")
        endif()
    # Option 3: JUCE from external path via environment variable
    elseif(DEFINED ENV{JUCE_DIR})
        set(JUCE_DIR "$ENV{JUCE_DIR}" PARENT_SCOPE)
        if(EXISTS "$ENV{JUCE_DIR}/CMakeLists.txt")
            add_subdirectory("$ENV{JUCE_DIR}")
            return()
        else()
            message(WARNING "JUCE_DIR set but CMakeLists.txt not found")
        endif()
    endif()

    # Option 4: Find JUCE package or fetch as a last resort
    find_package(JUCE QUIET)
    if(JUCE_FOUND)
        return()
    endif()

    include(FetchContent)
    if(NOT juce_POPULATED)
        message(STATUS "Fetching JUCE via FetchContent (tag 7.0.11)")
        FetchContent_Declare(
            juce
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 7.0.11
        )
        FetchContent_MakeAvailable(juce)
    endif()
endfunction()

