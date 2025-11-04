#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct MCVertex; // forward
struct MCTriangle; // forward

bool write_vtk_legacy_polydata(const std::string &path,
                               const std::vector<MCVertex> &vertices,
                               const std::vector<MCTriangle> &triangles,
                               std::string &errorMessage);


