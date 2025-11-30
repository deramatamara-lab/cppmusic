# Source code modules

# Core module (must be first for dependencies)
add_subdirectory(core)

# Audio module
add_subdirectory(audio)

# UI module
add_subdirectory(ui)

# Integration module (connects audio, UI, and core)
add_subdirectory(integration)

# AI module
add_subdirectory(ai)

# Project module
add_subdirectory(project)

# Plugins module
add_subdirectory(plugins)

# Main application (optional)
if(BUILD_MAIN_APP)
    add_subdirectory(main)
endif()

