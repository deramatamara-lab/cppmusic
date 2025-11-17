cmake_minimum_required(VERSION 3.22)

project(DAWProject VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard to C++20 (as per development rules)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler warnings - treat as errors (as per development rules)
if(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Setup JUCE
include(cmake/JUCE.cmake)
setup_juce()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

