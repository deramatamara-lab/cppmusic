# UI Animation module - Professional animation engine

set(UI_ANIMATION_SOURCES
    ProfessionalAnimationEngine.cpp
    AdaptiveAnimationService.cpp
    AdaptiveAnimationManager.cpp
    Animation.cpp
)

set(UI_ANIMATION_HEADERS
    ProfessionalAnimationEngine.h
    AdaptiveAnimationService.h
    AdaptiveAnimationManager.h
    Animation.hpp
)

add_library(ui_animation STATIC
    ${UI_ANIMATION_SOURCES}
    ${UI_ANIMATION_HEADERS}
)

target_include_directories(ui_animation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Disable web browser component to avoid GTK/WebKit dependency
target_compile_definitions(ui_animation PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

# Ensure GTK headers are available for juce_gui_basics on Linux
if(UNIX AND NOT APPLE)
    # Try pkg-config first; fall back to standard GTK-3 include path
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GTK3 QUIET gtk+-3.0)
    endif()

    if(GTK3_INCLUDE_DIRS)
        target_include_directories(ui_animation PRIVATE ${GTK3_INCLUDE_DIRS})
    else()
        # Fallback: standard GTK 3 include path on Debian/Ubuntu
        target_include_directories(ui_animation PRIVATE /usr/include/gtk-3.0)
    endif()
endif()

if(TARGET juce::juce_gui_basics)
    target_link_libraries(ui_animation PUBLIC
        juce::juce_gui_basics
        juce::juce_core
        juce::juce_opengl
    )
else()
    message(WARNING "ui_animation built without JUCE GUI modules; functionality will be limited")
endif()

