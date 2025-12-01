# UI tests

# Example test structure - uncomment when ready
# add_executable(ui_tests
#     WaveformViewerTest.cpp
#     ControlPanelTest.cpp
# )
# 
# target_link_libraries(ui_tests PRIVATE
#     ui_components
#     ui_lookandfeel
# )
# 
# add_test(NAME UITests COMMAND ui_tests)

# ==============================================================================
# GODMODE UI Tests (only if ImGui is enabled)
# ==============================================================================

# Check if ImGui UI targets exist
if(TARGET cppmusic_godmode_ui)
    # Reactive signal tests
    add_executable(test_reactive_signals
        reactive/TestReactiveSignals.cpp
    )
    target_include_directories(test_reactive_signals PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(test_reactive_signals PRIVATE
        cppmusic_godmode_ui
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_reactive_signals PRIVATE 
            -Wno-conversion 
            -Wno-sign-conversion
        )
    endif()
    add_test(NAME ReactiveSignalsTest COMMAND test_reactive_signals)

    # Lua sandbox tests
    add_executable(test_lua_sandbox
        script/TestLuaSandbox.cpp
    )
    target_include_directories(test_lua_sandbox PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(test_lua_sandbox PRIVATE
        cppmusic_godmode_ui
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_lua_sandbox PRIVATE 
            -Wno-conversion 
            -Wno-sign-conversion
        )
    endif()
    add_test(NAME LuaSandboxTest COMMAND test_lua_sandbox)
endif()
