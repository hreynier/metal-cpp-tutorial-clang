# Metal Tutorial C++ Clang Port

A port of the [metal-tutorial](https://metal-tutorial.com/) series in C++ but using CLang rather than XCode.

## Quick Start

You can clone and run the project below. Make sure to switch to specific tagged commits to run the different sections.
E.g lesson-01 will run the minecraft square, lesson-02 will run the 3D cube etc

```bash
git clone --recursive https://github.com/hreynier/minimal-metal-cpp.git
cd minimal-metal-cpp
./build.sh run
```

## Prerequisites

- macOS (Metal is Apple-exclusive)
- CMake 3.28.0 or later
- Xcode Command Line Tools: `xcode-select --install`

## Build & Run

```bash
# Using build script (recommended)
./build.sh run          # Build and run
./build.sh clean        # Clean rebuild
./build.sh              # Build only

# Using CMake directly
mkdir -p build && cd build
cmake .. && cmake --build . --verbose
./minimal-metal-cpp

# Using Make
make run                # Build and run
make clean              # Clean build
```

## Project Structure

```
src/
├── main.cpp                 # Entry point
├── mtl_engine.hpp/.cpp      # Metal rendering engine
├── mtl_implementation.cpp   # Metal-cpp bindings
└── shaders/*.metal   # Vertex & fragment shaders
```
