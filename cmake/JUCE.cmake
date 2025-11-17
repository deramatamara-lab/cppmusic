# JUCE CMake integration helper
# This file provides helper functions for integrating JUCE into the build system

# Example function to find or add JUCE
function(setup_juce)
    # Option 1: JUCE checked out at repository root
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" PARENT_SCOPE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
            add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
        else()
            message(WARNING "JUCE directory found but CMakeLists.txt not present")
        endif()
    # Option 2: JUCE cloned into external/JUCE (no git submodule required)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE")
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE" PARENT_SCOPE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
            add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE")
        else()
            message(WARNING "external/JUCE directory found but CMakeLists.txt not present")
        endif()
    # Option 3: JUCE from external path via environment variable
    elseif(DEFINED ENV{JUCE_DIR})
        set(JUCE_DIR "$ENV{JUCE_DIR}" PARENT_SCOPE)
        if(EXISTS "$ENV{JUCE_DIR}/CMakeLists.txt")
            add_subdirectory("$ENV{JUCE_DIR}")
        else()
            message(WARNING "JUCE_DIR set but CMakeLists.txt not found")
        endif()
    # Option 4: Find JUCE package
    else()
        find_package(JUCE QUIET)
        if(NOT JUCE_FOUND)
            message(WARNING "JUCE not found. Please set JUCE_DIR or add JUCE as submodule.")
        endif()
    endif()
endfunction()

