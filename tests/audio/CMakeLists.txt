# Audio tests

add_subdirectory(engine)
add_subdirectory(dsp)

add_executable(audio_tests
    engine/TransportTest.cpp
    engine/DawEngineTest.cpp
    dsp/TrackStripTest.cpp
    main.cpp
)

target_link_libraries(audio_tests PRIVATE
    audio_engine
    audio_dsp
    audio_effects
    audio_processors
)

if(TARGET juce::juce_core)
    target_link_libraries(audio_tests PRIVATE
        juce::juce_core
        juce::juce_audio_processors
        juce::juce_audio_devices
    )
endif()

add_test(NAME AudioTests COMMAND audio_tests)

