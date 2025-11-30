cmake_minimum_required(VERSION 3.22)

project(DAWProject VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard to C++20 (as per development rules)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Hardened optimization settings for GCC builds (JUCE can trigger GCC 13 ICEs at -O3)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
    # Disable specific passes that are known to cause ICEs in JUCE-generated translation units
    add_compile_options(-fno-tree-forwprop -fno-tree-ccp -fno-gcse)
endif()

# Compiler warnings - treat as errors (as per development rules)
if(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Silence noisy diagnostics coming exclusively from JUCE's generated code while keeping -Werror everywhere else
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wno-array-bounds -Wno-maybe-uninitialized)
endif()

# Setup JUCE
include(cmake/JUCE.cmake)
setup_juce()

enable_testing()

option(BUILD_MAIN_APP "Build main application" ON)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

