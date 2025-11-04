#pragma once

#include <array>
#include <cstdint>
#include <vector>

struct MCVertex { float x, y, z; };
struct MCTriangle { uint32_t a, b, c; };

// Generate triangle mesh for the isosurface.
// Input volume layout: index = z*(ny*nx) + y*nx + x, dimensions (nx, ny, nz)
void marching_cubes(const float *volume,
                    int nx, int ny, int nz,
                    float isoValue,
                    std::vector<MCVertex> &outVertices,
                    std::vector<MCTriangle> &outTriangles);


