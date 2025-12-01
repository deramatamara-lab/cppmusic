# Test configuration

enable_testing()

# Add test subdirectories
add_subdirectory(audio)
add_subdirectory(ui)
add_subdirectory(ai)
add_subdirectory(core)
add_subdirectory(project)

# Unit tests for engine skeleton and model layer
add_subdirectory(unit)

# Optional: Add testing framework (e.g., Catch2, Google Test, or JUCE UnitTest)
# find_package(Catch2 REQUIRED)

