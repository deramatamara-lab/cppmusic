# Source code modules

# Core module (must be first for dependencies)
add_subdirectory(core)

# Engine skeleton (foundational audio graph and transport)
add_subdirectory(engine)

# Model layer (pattern, note events)
add_subdirectory(model)

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

# Services module (undo, collab, integrity)
add_subdirectory(services)

# Platform module (sandbox, plugin inspection)
add_subdirectory(platform)

# Main application (optional)
if(BUILD_MAIN_APP)
    add_subdirectory(main)
endif()

