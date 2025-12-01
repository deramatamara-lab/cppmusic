add_library(concurrency STATIC
    AsyncDispatcher.cpp
)

target_include_directories(concurrency PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
