# Services module

add_subdirectory(undo)
add_subdirectory(integrity)

if(ENABLE_COLLAB)
    add_subdirectory(collab)
endif()

# Dispatch service (if exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dispatch/CMakeLists.txt")
    add_subdirectory(dispatch)
endif()
