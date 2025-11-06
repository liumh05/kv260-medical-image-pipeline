# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a pure C++ implementation of the Marching Cubes algorithm for 3D surface reconstruction from volumetric data. The project:
- Takes NPY files (NumPy array format) with shape (1, D, H, W) as input
- Supports multiple data types: float32, int16, int32, uint8
- Outputs VTK legacy PolyData files (.vtk) containing the extracted isosurface mesh
- Has no third-party dependencies

## Build Commands

```bash
# Create build directory and configure
mkdir build && cd build
cmake ..

# Build the project (using all available cores)
cmake --build . -j

# Alternative: Build in debug mode
cmake -DCMAKE_BUILD_TYPE=Debug .. && cmake --build . -j
```

## Running the Program

```bash
# Basic usage
./marching_cubes_c --input /path/to/volume.npy --iso 300.0 --vtk output.vtk

# With statistics
./marching_cubes_c --input volume.npy --iso 0.5 --vtk output.vtk --stats

# Show help
./marching_cubes_c --help
```

**Required arguments:**
- `--input <path>`: Input NPY file (must have shape (1, D, H, W))
- `--vtk <path>`: Output VTK file path
- `--iso <value>`: Isovalue for surface extraction (float)

**Optional arguments:**
- `--stats`: Print volume statistics (min, max, mean)

## Architecture

The codebase is organized into four main components:

### Core Modules

1. **NPY Reader** (`src/npy_reader.h/cpp`): Minimal NumPy array loader that parses NPY headers and converts supported data types to float32 for processing

2. **Marching Cubes Algorithm** (`src/marching_cubes.h/cpp`): Implements the classic Marching Cubes algorithm using edge tables and triangle tables to generate triangle meshes from volumetric data

3. **VTK Writer** (`src/vtk_writer.h/cpp`): Outputs triangle meshes in VTK legacy PolyData format for visualization in tools like ParaView

4. **Main Entry Point** (`src/main.cpp`): Command-line interface that orchestrates the pipeline from NPY input to VTK output

### Data Structures

- `MCVertex`: 3D vertex position (float x, y, z)
- `MCTriangle`: Triangle face defined by three vertex indices (uint32_t a, b, c)

### Volume Layout

The volume data uses C-order memory layout: `index = z*(ny*nx) + y*nx + x` where dimensions are (nx, ny, nz) corresponding to width, height, depth respectively.

## Development Notes

- The project uses C++17 standard
- Compiler warnings are enabled for GCC/Clang (-Wall -Wextra -Wpedantic)
- No external dependencies - all functionality is self-contained
- The root `main.cpp` appears to be a CLion IDE template and is not used in the build
- Build artifacts are generated in `build/` and `cmake-build-debug/` directories