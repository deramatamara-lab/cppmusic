# CMake configuration for Dear ImGui with SDL2 + OpenGL backend
# Provides the cppmusic_imgui_ui library for the ultra-polished UI shell

include(FetchContent)

# ==============================================================================
# Option to enable/disable ImGui UI
# ==============================================================================
option(ENABLE_IMGUI_UI "Enable Dear ImGui-based UI" ON)

if(NOT ENABLE_IMGUI_UI)
    message(STATUS "ImGui UI disabled, skipping build")
    return()
endif()

# ==============================================================================
# Find SDL2
# ==============================================================================
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SDL2 sdl2)
endif()

if(NOT SDL2_FOUND)
    find_package(SDL2 QUIET)
endif()

if(NOT SDL2_FOUND AND NOT SDL2_LIBRARIES)
    message(STATUS "SDL2 not found - ImGui UI will not be built")
    set(ENABLE_IMGUI_UI OFF PARENT_SCOPE)
    return()
endif()

# ==============================================================================
# Find OpenGL
# ==============================================================================
find_package(OpenGL REQUIRED)

# ==============================================================================
# Fetch Dear ImGui (docking branch)
# ==============================================================================
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    TRUE
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# ==============================================================================
# Create ImGui library with SDL2 + OpenGL3 backends
# ==============================================================================
add_library(imgui_backend STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui_backend PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui_backend PUBLIC
    ${SDL2_LIBRARIES}
    OpenGL::GL
)

target_include_directories(imgui_backend PUBLIC
    ${SDL2_INCLUDE_DIRS}
)

# ==============================================================================
# ImGui UI Library Sources
# ==============================================================================
set(IMGUI_UI_SOURCES
    App.cpp
    Theme.cpp
    Shortcuts.cpp
    audio/AudioEngine.cpp
    panels/TransportBar.cpp
    panels/BrowserPanel.cpp
    panels/ChannelRackPanel.cpp
    panels/PianoRollPanel.cpp
    panels/PlaylistPanel.cpp
    panels/MixerPanel.cpp
    panels/InspectorPanel.cpp
    panels/PluginWindow.cpp
)

set(IMGUI_UI_HEADERS
    App.hpp
    Theme.hpp
    Shortcuts.hpp
    audio/AudioEngine.hpp
    panels/TransportBar.hpp
    panels/BrowserPanel.hpp
    panels/ChannelRackPanel.hpp
    panels/PianoRollPanel.hpp
    panels/PlaylistPanel.hpp
    panels/MixerPanel.hpp
    panels/InspectorPanel.hpp
    panels/PluginWindow.hpp
)

add_library(cppmusic_imgui_ui STATIC
    ${IMGUI_UI_SOURCES}
    ${IMGUI_UI_HEADERS}
)

target_include_directories(cppmusic_imgui_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(cppmusic_imgui_ui PUBLIC
    imgui_backend
    ${SDL2_LIBRARIES}
    OpenGL::GL
)

# Warning configuration:
# - imgui_backend: Third-party ImGui code has many warnings with strict flags,
#   disabled to avoid noise while still catching issues in our own code.
# - cppmusic_imgui_ui: Only minimal warnings disabled (conversion) since ImGui
#   uses many implicit conversions in its API that would require verbose casts.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Third-party ImGui code - disable all strict warnings
    target_compile_options(imgui_backend PRIVATE
        -Wno-conversion
        -Wno-sign-conversion
        -Wno-float-conversion
        -Wno-implicit-int-float-conversion
        -Wno-double-promotion
        -Wno-deprecated-enum-enum-conversion
        -Wno-old-style-cast
    )
    # Our code - keep most strict warnings, only disable ImGui API conversion issues
    target_compile_options(cppmusic_imgui_ui PRIVATE
        -Wno-conversion
        -Wno-sign-conversion
    )
endif()

# ==============================================================================
# Sample App Executable (optional)
# ==============================================================================
option(BUILD_IMGUI_SAMPLE_APP "Build ImGui sample application" ON)

if(BUILD_IMGUI_SAMPLE_APP)
    add_executable(cppmusic_imgui_app
        main.cpp
    )

    target_link_libraries(cppmusic_imgui_app PRIVATE
        cppmusic_imgui_ui
    )

    # Set runtime output directory
    set_target_properties(cppmusic_imgui_app PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()

# ==============================================================================
# Headless Smoke Test
# ==============================================================================
option(BUILD_IMGUI_SMOKE_TEST "Build ImGui headless smoke test" ON)

if(BUILD_IMGUI_SMOKE_TEST)
    add_executable(imgui_smoke_test
        tests/smoke_test.cpp
    )

    target_link_libraries(imgui_smoke_test PRIVATE
        cppmusic_imgui_ui
    )

    add_test(NAME ImGuiSmokeTest COMMAND imgui_smoke_test)
endif()

# ==============================================================================
# GODMODE UI Components
# ==============================================================================
option(ENABLE_LUA_SCRIPTING "Enable Lua scripting support" OFF)

# GODMODE Core Library - Reactive, Input, Render, Diagnostics, Layout
add_library(cppmusic_godmode_ui STATIC
    # Reactive data binding
    ${CMAKE_SOURCE_DIR}/src/ui/core/reactive/Signal.hpp
    ${CMAKE_SOURCE_DIR}/src/ui/core/reactive/ParameterSignal.hpp

    # Input routing
    ${CMAKE_SOURCE_DIR}/src/ui/core/input/InputRouter.hpp

    # GPU rendering
    ${CMAKE_SOURCE_DIR}/src/ui/core/render/WaveformRenderer.hpp
    ${CMAKE_SOURCE_DIR}/src/ui/core/render/WaveformRenderer.cpp

    # Diagnostics
    ${CMAKE_SOURCE_DIR}/src/ui/core/diagnostics/DiagnosticsOverlay.hpp
    ${CMAKE_SOURCE_DIR}/src/ui/core/diagnostics/DiagnosticsOverlay.cpp

    # Layout persistence
    ${CMAKE_SOURCE_DIR}/src/ui/core/layout/LayoutSerializer.hpp
    ${CMAKE_SOURCE_DIR}/src/ui/core/layout/LayoutSerializer.cpp

    # Theme system
    ${CMAKE_SOURCE_DIR}/src/ui/theme/ThemeManager.hpp
    ${CMAKE_SOURCE_DIR}/src/ui/theme/ThemeManager.cpp

    # Internationalization
    ${CMAKE_SOURCE_DIR}/src/ui/i18n/I18n.hpp
    ${CMAKE_SOURCE_DIR}/src/ui/i18n/I18n.cpp

    # Lua scripting (stub implementation)
    ${CMAKE_SOURCE_DIR}/src/ui/script/LuaVM.hpp
    ${CMAKE_SOURCE_DIR}/src/ui/script/LuaVM.cpp
)

target_include_directories(cppmusic_godmode_ui PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(cppmusic_godmode_ui PUBLIC
    imgui_backend
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cppmusic_godmode_ui PRIVATE
        -Wno-conversion
        -Wno-sign-conversion
    )
endif()

# Optional: Full Lua integration
if(ENABLE_LUA_SCRIPTING)
    find_package(Lua 5.4 QUIET)
    if(NOT LUA_FOUND)
        message(STATUS "Lua not found, using FetchContent")
        FetchContent_Declare(
            lua
            GIT_REPOSITORY https://github.com/lua/lua.git
            GIT_TAG        v5.4.6
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(lua)
    endif()
    target_compile_definitions(cppmusic_godmode_ui PUBLIC ENABLE_LUA_SCRIPTING=1)
endif()

# Link GODMODE to main ImGui UI
target_link_libraries(cppmusic_imgui_ui PUBLIC cppmusic_godmode_ui)
