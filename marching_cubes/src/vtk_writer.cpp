#include "vtk_writer.h"
#include "marching_cubes.h"

#include <fstream>

bool write_vtk_legacy_polydata(const std::string &path,
                               const std::vector<MCVertex> &vertices,
                               const std::vector<MCTriangle> &triangles,
                               std::string &errorMessage) {
    std::ofstream out(path);
    if (!out) { errorMessage = "无法写入VTK文件: " + path; return false; }
    out << "# vtk DataFile Version 3.0\n";
    out << "marching cubes output\n";
    out << "ASCII\n";
    out << "DATASET POLYDATA\n";
    out << "POINTS " << vertices.size() << " float\n";
    for (const auto &v : vertices) {
        out << v.x << " " << v.y << " " << v.z << "\n";
    }
    size_t nTri = triangles.size();
    out << "POLYGONS " << nTri << " " << nTri * 4 << "\n";
    for (const auto &t : triangles) {
        out << 3 << " " << t.a << " " << t.b << " " << t.c << "\n";
    }
    return true;
}


