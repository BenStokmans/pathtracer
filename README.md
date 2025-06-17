# PathTracer

A Metal-based pathtracer for macOS using metal-cpp.

## Building and Running

This project uses a clean build system that keeps all build artifacts in a separate `build/` directory.

### Scripts

All build scripts are located in the `scripts/` directory:

- **`./scripts/build.sh`** - Build the project (creates `build/` directory)
- **`./scripts/run.sh`** - Build (if needed) and run the application
- **`./scripts/clean.sh`** - Remove all build artifacts and clean the project
- **`./scripts/debug.sh`** - Build in debug mode and launch with lldb debugger


## Requirements

- macOS 10.15+
- Xcode Command Line Tools
- CMake 3.20+
- Metal-compatible GPU
