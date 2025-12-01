# Contributing Guidelines

Welcome to the cppmusic DAW project! This document provides guidelines for contributing to the project.

## Build Instructions (Ubuntu/Linux)

### Prerequisites

Install the required dependencies:

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    clang \
    cmake \
    ninja-build \
    pkg-config \
    libsndfile1-dev \
    libx11-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxinerama-dev \
    libxext-dev \
    libfreetype6-dev \
    libcurl4-openssl-dev \
    libasound2-dev
```

### Building

```bash
# Clone the repository
git clone <repository-url>
cd cppmusic

# Create build directory
mkdir build && cd build

# Configure (Debug build with sanitizers)
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..

# Build
cmake --build .

# Run tests
ctest --output-on-failure
```

### Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `ENABLE_ASAN` | Enable AddressSanitizer | OFF |
| `ENABLE_UBSAN` | Enable UndefinedBehaviorSanitizer | OFF |
| `ENABLE_LTO` | Enable Link Time Optimization | OFF |
| `ENABLE_LOW_LATENCY` | Enable low-latency flags (-ffast-math -fno-exceptions -fno-rtti) | OFF |
| `ENABLE_JUCE` | Enable JUCE framework | ON |
| `ENABLE_PORTAUDIO` | Enable PortAudio backend | OFF |
| `BUILD_MAIN_APP` | Build main application | ON |

## Branching Strategy

We follow a structured branching model:

- **`main`**: Stable, release-ready code. All CI checks must pass.
- **`dev`**: Integration branch for feature development.
- **`feature/*`**: Feature branches for new functionality (e.g., `feature/midi-input`).
- **`bugfix/*`**: Bug fix branches (e.g., `bugfix/audio-glitch`).
- **`hotfix/*`**: Urgent fixes for production issues.

### Workflow

1. Create a feature branch from `dev`: `git checkout -b feature/my-feature dev`
2. Develop and commit your changes
3. Open a Pull Request to `dev`
4. After review and CI passes, merge to `dev`
5. Periodically, `dev` is merged to `main` for releases

## Development Workflow

1. **Read the Rules**: Understand [DAW_DEV_RULES.md](DAW_DEV_RULES.md) - **MANDATORY**
2. **Create Branch**: Create feature branch from `dev`
3. **Write Code**: Follow all development rules
4. **Write Tests**: Add unit tests for new functionality
5. **Run Locally**: Build and test before pushing
6. **Code Review**: Ensure all checklist items pass
7. **Merge**: After approval, merge to `dev`

## Code Style

### Formatting

- Use the `.clang-format` file in the repository root
- Run `clang-format -i <file>` before committing
- EditorConfig settings in `.editorconfig` ensure consistent whitespace

### Naming Conventions

- **Classes**: `PascalCase` (e.g., `AudioGraph`, `Transport`)
- **Functions**: `camelCase` (e.g., `processBlock`, `getPositionBeats`)
- **Variables**: `camelCase` (e.g., `sampleRate`, `numChannels`)
- **Constants**: `kPascalCase` or `SCREAMING_SNAKE_CASE` (e.g., `kDefaultTempo`)
- **Private Members**: Trailing underscore (e.g., `sampleRate_`)

### Header Guards

Use `#pragma once` for header guards.

## Testing Requirements

- **Audio Processing**: >80% code coverage
- **Real-time Safety**: Verify no allocations in `processBlock()`
- **Thread Safety**: Test concurrent access patterns
- **UI Components**: Test responsiveness and accessibility
- **Run CTest**: `ctest --output-on-failure` must pass

## Commit Convention (Conventional Commits)

We follow [Conventional Commits](https://www.conventionalcommits.org/) for clear history:

```
<type>(<scope>): <description>

[optional body]

[optional footer(s)]
```

### Types

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, no logic change)
- `refactor`: Code refactoring (no feature or fix)
- `perf`: Performance improvement
- `test`: Adding or updating tests
- `build`: Build system changes
- `ci`: CI/CD changes
- `chore`: Other changes (deps, tooling)

### Examples

```
feat(engine): add sample-accurate transport position

- Implement advancePosition() for audio thread
- Add beat-to-sample conversion utilities
- Include unit tests for Transport class
```

```
fix(audio): prevent buffer overflow in processBlock

Ensure numSamples does not exceed buffer capacity.

Fixes #123
```

## Pull Request Checklist

Before submitting a PR, ensure:

- [ ] Code compiles without warnings (`-Wall -Wextra -Wpedantic`)
- [ ] All tests pass (`ctest --output-on-failure`)
- [ ] No raw pointers or manual memory management
- [ ] No locks or allocations in audio thread (`processBlock()`)
- [ ] All public APIs documented with Doxygen comments
- [ ] Unit tests written for new functionality
- [ ] Follows SOLID principles
- [ ] Proper error handling
- [ ] Thread safety verified
- [ ] Code formatted with `.clang-format`
- [ ] Commit messages follow Conventional Commits
- [ ] PR description explains the changes

## Code Review Checklist

Before submitting code for review, verify:

- [ ] No raw pointers or manual memory management
- [ ] No locks or allocations in audio thread (`processBlock()`)
- [ ] All public APIs documented with Doxygen comments
- [ ] Unit tests written and passing
- [ ] Follows SOLID principles
- [ ] Proper error handling
- [ ] Thread safety verified
- [ ] Performance profiled (if applicable)
- [ ] UI is accessible and responsive
- [ ] No hardcoded values (use constants)
- [ ] Code formatted with `.clang-format`
- [ ] No compiler warnings
- [ ] Cross-platform compatibility verified

## Coding Standards

### Memory Management

```cpp
// ✅ DO: Use smart pointers
auto processor = std::make_unique<AudioProcessor>();

// ❌ DON'T: Raw pointers
AudioProcessor* processor = new AudioProcessor();
```

### Audio Thread Safety

```cpp
// ✅ DO: Lock-free access
auto gain = gainAtomic.load(std::memory_order_acquire);

// ❌ DON'T: Locks in audio thread
const ScopedLock sl(lock);  // FORBIDDEN
```

### AI Processing

```cpp
// ✅ DO: Async AI processing
aiProcessor.requestAIProcessing(input);

// ❌ DON'T: AI in audio thread
auto result = aiModel.infer(buffer);  // FORBIDDEN
```

## Documentation

- All public classes and methods must have Doxygen comments
- Include parameter descriptions
- Document thread safety guarantees
- Provide usage examples for complex APIs

## Getting Help

- Open a GitHub Issue for bugs or feature requests
- Use GitHub Discussions for questions
- Read the [Architecture Documentation](ARCHITECTURE.md) for system overview
- Check the [Roadmap](ROADMAP.md) for planned features

