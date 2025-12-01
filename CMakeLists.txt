cmake_minimum_required(VERSION 3.22)

project(DAWProject VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard to C++20 (as per development rules)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Build Options
# ==============================================================================
option(BUILD_MAIN_APP "Build main application" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_LTO "Enable Link Time Optimization" OFF)
option(ENABLE_JUCE "Enable JUCE framework (stub for future use)" ON)
option(ENABLE_PORTAUDIO "Enable PortAudio backend (stub for future use)" OFF)

# ==============================================================================
# Include centralized toolchain flags
# ==============================================================================
include(cmake/ToolchainFlags.cmake)

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

# ==============================================================================
# JUCE Setup (conditional)
# ==============================================================================
if(ENABLE_JUCE)
    include(cmake/JUCE.cmake)
    setup_juce()

    # Disable JUCE web browser components to avoid WebKit dependency on Linux
    # These are project-wide definitions so all targets inherit them
    add_compile_definitions(
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )
endif()

# ==============================================================================
# PortAudio Setup (stub for future use)
# ==============================================================================
if(ENABLE_PORTAUDIO)
    message(STATUS "PortAudio support: stub enabled (not yet implemented)")
    # TODO: Add PortAudio integration when ready
endif()

# ==============================================================================
# Testing
# ==============================================================================
enable_testing()

# ==============================================================================
# Add clang-tidy target
# ==============================================================================
cppmusic_add_clang_tidy_target()

# ==============================================================================
# Add subdirectories
# ==============================================================================
add_subdirectory(src)
add_subdirectory(tests)

