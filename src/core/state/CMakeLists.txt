# Core State Management

set(CORE_STATE_SOURCES
    ApplicationState.cpp
    ParameterManager.cpp
)

set(CORE_STATE_HEADERS
    ApplicationState.h
    ParameterManager.h
)

add_library(core_state STATIC
    ${CORE_STATE_SOURCES}
    ${CORE_STATE_HEADERS}
)

target_include_directories(core_state PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core_state PUBLIC
    core_utilities
)

