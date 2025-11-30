# Test Suite

This directory contains unit tests for the DAW project using JUCE's UnitTest framework.

## Test Structure

### Audio Tests (`tests/audio/`)
- **TransportTest**: Tests transport control (play/stop, tempo, position, time signature)
- **DawEngineTest**: Tests engine functionality (track management, parameters, metering, CPU load)
- **TrackStripTest**: Tests track strip DSP (gain, pan, mute, solo, audio processing)

### Project Model Tests (`tests/project/`)
- **ProjectModelTest**: Tests project model (tracks, clips, patterns, associations, mixer parameters, selection)
- **UndoManagerTest**: Tests undo/redo system (command execution, history management)

### AI Tests (`tests/ai/`)
- **InferenceEngineTest**: Tests AI inference engine (initialization, queueing, bounded queue, shutdown)

## Building Tests

Tests require JUCE to be properly configured. To build tests without the main application:

```bash
cmake -S . -B build-tests -DCMAKE_BUILD_TYPE=Debug -DBUILD_MAIN_APP=OFF
cd build-tests
make
```

## Running Tests

Once built, run tests using CTest:

```bash
cd build-tests
ctest --output-on-failure
```

Or run individual test executables:

```bash
./tests/audio/audio_tests
./tests/project/project_tests
./tests/ai/ai_tests
```

## Test Coverage Goals

- **Audio Engine**: 80%+ coverage (critical for real-time safety)
- **Project Model**: 80%+ coverage (core data structures)
- **AI System**: Basic functionality tests

## Notes

- Tests use JUCE's `UnitTest` framework
- All tests follow the pattern: `class XxxTest : public juce::UnitTest`
- Tests are registered as static instances: `static XxxTest xxxTest;`
- Tests validate both happy paths and edge cases

