# AI module

add_subdirectory(models)
add_subdirectory(inference)
add_subdirectory(config)

# Mega-integration AI subsystems
if(ENABLE_AI)
    add_subdirectory(harmony)
    add_subdirectory(tagging)
    add_subdirectory(arrangement)
endif()
