# cmake/ToolchainFlags.cmake
# Centralized compiler, linker, security, and performance flags for the cppmusic DAW project.
# Target: Ubuntu (Linux) production readiness.

# ==============================================================================
# Compiler Detection
# ==============================================================================
set(CPPMUSIC_COMPILER_IS_GNU FALSE)
set(CPPMUSIC_COMPILER_IS_CLANG FALSE)
set(CPPMUSIC_COMPILER_IS_MSVC FALSE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CPPMUSIC_COMPILER_IS_GNU TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CPPMUSIC_COMPILER_IS_CLANG TRUE)
elseif(MSVC)
    set(CPPMUSIC_COMPILER_IS_MSVC TRUE)
endif()

# ==============================================================================
# Warning Flags (enforced for all introduced code)
# ==============================================================================
function(cppmusic_add_warning_flags target)
    if(CPPMUSIC_COMPILER_IS_GNU OR CPPMUSIC_COMPILER_IS_CLANG)
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
            -Wnull-dereference
        )
    elseif(CPPMUSIC_COMPILER_IS_MSVC)
        target_compile_options(${target} PRIVATE
            /W4
        )
    endif()
endfunction()

# ==============================================================================
# Security Hardening Flags
# ==============================================================================
function(cppmusic_add_security_flags target)
    if(CPPMUSIC_COMPILER_IS_GNU OR CPPMUSIC_COMPILER_IS_CLANG)
        target_compile_options(${target} PRIVATE
            -fstack-protector-strong
            -fPIE
        )
        target_compile_definitions(${target} PRIVATE
            _FORTIFY_SOURCE=2
        )
        target_link_options(${target} PRIVATE
            -Wl,-z,relro,-z,now
            -pie
        )
    endif()
endfunction()

# ==============================================================================
# Performance / LTO Flags
# ==============================================================================
function(cppmusic_add_lto_flags target)
    if(NOT ENABLE_LTO)
        return()
    endif()
    if(CPPMUSIC_COMPILER_IS_GNU OR CPPMUSIC_COMPILER_IS_CLANG)
        target_compile_options(${target} PRIVATE -flto)
        target_link_options(${target} PRIVATE -flto)
    elseif(CPPMUSIC_COMPILER_IS_MSVC)
        target_compile_options(${target} PRIVATE /GL)
        target_link_options(${target} PRIVATE /LTCG)
    endif()
endfunction()

# ==============================================================================
# Sanitizer Flags
# ==============================================================================
function(cppmusic_add_sanitizer_flags target)
    if(ENABLE_ASAN)
        if(CPPMUSIC_COMPILER_IS_GNU OR CPPMUSIC_COMPILER_IS_CLANG)
            target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address)
        endif()
    endif()

    if(ENABLE_UBSAN)
        if(CPPMUSIC_COMPILER_IS_GNU OR CPPMUSIC_COMPILER_IS_CLANG)
            target_compile_options(${target} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=undefined)
        endif()
    endif()
endfunction()

# ==============================================================================
# Low-Latency Flags (opt-in, evaluate tradeoffs)
# ==============================================================================
function(cppmusic_add_low_latency_flags target)
    if(NOT ENABLE_LOW_LATENCY)
        return()
    endif()
    if(CPPMUSIC_COMPILER_IS_GNU OR CPPMUSIC_COMPILER_IS_CLANG)
        target_compile_options(${target} PRIVATE
            -ffast-math
            -fno-exceptions
            -fno-rtti
        )
        message(STATUS "Low-latency flags enabled for ${target}: -ffast-math -fno-exceptions -fno-rtti")
    endif()
endfunction()

# ==============================================================================
# Combined Function: Apply all flags to a target
# ==============================================================================
function(cppmusic_apply_all_flags target)
    cppmusic_add_warning_flags(${target})
    cppmusic_add_security_flags(${target})
    cppmusic_add_lto_flags(${target})
    cppmusic_add_sanitizer_flags(${target})
    cppmusic_add_low_latency_flags(${target})
endfunction()

# ==============================================================================
# Clang-Tidy Target (uses existing .clang-tidy configuration)
# ==============================================================================
function(cppmusic_add_clang_tidy_target)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16 clang-tidy-15)
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")

        # Collect source files for clang-tidy
        file(GLOB_RECURSE CPPMUSIC_SOURCES
            "${CMAKE_SOURCE_DIR}/src/engine/*.cpp"
            "${CMAKE_SOURCE_DIR}/src/engine/*.hpp"
            "${CMAKE_SOURCE_DIR}/src/model/*.cpp"
            "${CMAKE_SOURCE_DIR}/src/model/*.hpp"
            "${CMAKE_SOURCE_DIR}/tests/unit/*.cpp"
        )

        if(CPPMUSIC_SOURCES)
            add_custom_target(clang-tidy
                COMMAND ${CLANG_TIDY_EXE}
                    -p ${CMAKE_BINARY_DIR}
                    ${CPPMUSIC_SOURCES}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Running clang-tidy on engine/model/tests sources..."
                VERBATIM
            )
        else()
            add_custom_target(clang-tidy
                COMMAND ${CMAKE_COMMAND} -E echo "No sources found for clang-tidy"
                COMMENT "No engine/model/tests sources to analyze"
            )
        endif()
    else()
        message(STATUS "clang-tidy not found, skipping clang-tidy target")
        add_custom_target(clang-tidy
            COMMAND ${CMAKE_COMMAND} -E echo "clang-tidy not available"
            COMMENT "clang-tidy is not installed"
        )
    endif()
endfunction()
