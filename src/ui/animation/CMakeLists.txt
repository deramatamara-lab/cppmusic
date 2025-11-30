# UI Animation module - Professional animation engine

set(UI_ANIMATION_SOURCES
    ProfessionalAnimationEngine.cpp
    AdaptiveAnimationService.cpp
    AdaptiveAnimationManager.cpp
)

set(UI_ANIMATION_HEADERS
    ProfessionalAnimationEngine.h
    AdaptiveAnimationService.h
    AdaptiveAnimationManager.h
)

add_library(ui_animation STATIC
    ${UI_ANIMATION_SOURCES}
    ${UI_ANIMATION_HEADERS}
)

target_include_directories(ui_animation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Ensure GTK headers are available for juce_gui_extra on Linux
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

    # Also check for WebKit2GTK (for juce_gui_extra web browser components)
    pkg_check_modules(WEBKIT2GTK QUIET webkit2gtk-4.1)
    if(WEBKIT2GTK_INCLUDE_DIRS)
        target_include_directories(ui_animation PRIVATE ${WEBKIT2GTK_INCLUDE_DIRS})
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

