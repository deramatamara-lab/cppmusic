# Building the Project

## Prerequisites

### Required

- **CMake 3.22+**: Build system generator
- **C++17 Compiler**: 
  - Windows: MSVC 2019 or later
  - macOS: Apple Clang 13+ (Xcode 14+) – tested on macOS Sonoma
  - Linux: GCC 11+ or Clang 13+ – tested on Ubuntu 22.04 LTS
- **JUCE Framework**: Either add as submodule or point `JUCE_DIR` to a prebuilt checkout

### Platform packages

| Platform | Package Manager Commands |
| --- | --- |
| **macOS** | `brew install cmake ninja pkg-config` |
| **Ubuntu 22.04+** | `sudo apt update && sudo apt install build-essential cmake ninja-build pkg-config libx11-dev libxrandr-dev libxcursor-dev libxinerama-dev libxext-dev libfreetype6-dev libcurl4-openssl-dev libasound2-dev` |

### Optional

- **JUCE Framework**: For full functionality
  - Can be added as git submodule
  - Or set `JUCE_DIR` environment variable

## Build Steps

### Windows (Visual Studio)

```powershell
# Create build directory
mkdir build
cd build

# Generate Visual Studio solution
cmake .. -G "Visual Studio 17 2022" -A x64

# Build Release configuration
cmake --build . --config Release

# Or open in Visual Studio
start DAWProject.sln
```

### macOS (Xcode or Ninja)

```bash
mkdir build
cd build

# Xcode project (recommended for debugging)
cmake .. -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build . --config Release

# or fast CLI build via Ninja
cmake -S .. -B . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Run the app
./DAWProject.app/Contents/MacOS/DAWProject
```

### Ubuntu (Ninja)

```bash
mkdir build
cd build
cmake -S .. -B . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Launch (X11/Wayland)
./DAWProject
```

> **Tip:** If you are on Wayland, force XWayland for JUCE by running `QT_QPA_PLATFORM=xcb ./DAWProject`.

## CMake Options

- `CMAKE_BUILD_TYPE`: Debug, Release, RelWithDebInfo, MinSizeRel
- `CMAKE_CXX_STANDARD`: C++ standard (default: 17)
- `BUILD_TESTING`: Enable tests (default: ON)

## Troubleshooting

### JUCE Not Found

If JUCE is not found:

1. Add JUCE as submodule:
   ```bash
   git submodule add https://github.com/juce-framework/JUCE.git JUCE
   ```

2. Or set environment variable:
   ```bash
   export JUCE_DIR=/path/to/JUCE
   ```

3. Or uncomment JUCE setup in root CMakeLists.txt

### Compiler Warnings as Errors

The project treats warnings as errors. To temporarily disable:

```bash
cmake .. -DCMAKE_CXX_FLAGS="-Wno-error"
```

But this is **not recommended** - fix the warnings instead!

