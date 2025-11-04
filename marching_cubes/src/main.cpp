#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include "npy_reader.h"
#include "marching_cubes.h"
#include "vtk_writer.h"

static void print_usage() {
    std::cout << "用法:\n"
              << "  marching_cubes --input /path/vol.npy --iso 0.5 --vtk out.vtk\n\n"
              << "参数:\n"
              << "  --input <path>  输入NPY文件，形状(1,D,H,W)\n"
              << "  --iso <value>   等值（浮点数）\n"
              << "  --vtk <path>    输出VTK legacy PolyData文件（必选）\n";
}

int main(int argc, char** argv) {
    std::string inputPath;
    std::string outVTK;
    float iso = 0.5f;
    bool printStats = false;

    for (int i=1; i<argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--input" && i+1 < argc) { inputPath = argv[++i]; }
        else if (arg == "--iso" && i+1 < argc) { iso = std::stof(argv[++i]); }
        else if (arg == "--vtk" && i+1 < argc) { outVTK = argv[++i]; }
        else if (arg == "--stats") { printStats = true; }
        else if (arg == "-h" || arg == "--help") { print_usage(); return 0; }
        else { std::cerr << "未知参数: " << arg << "\n"; print_usage(); return 1; }
    }

    if (inputPath.empty()) { std::cerr << "必须提供--input\n"; print_usage(); return 1; }
    if (outVTK.empty()) { std::cerr << "必须提供--vtk\n"; print_usage(); return 1; }

    // Load npy to float
    std::vector<size_t> shape; std::vector<float> vol; std::string err;
    if (!load_npy_to_float(inputPath, shape, vol, err)) {
        std::cerr << "读取NPY失败: " << err << "\n"; return 1;
    }

    if (shape.size() != 4 || shape[0] != 1) {
        std::cerr << "期望形状为(1,D,H,W)，实际: ";
        for (size_t i=0;i<shape.size();++i) std::cerr << (i?",":"(") << shape[i];
        std::cerr << ")\n";
        return 1;
    }
    int nz = static_cast<int>(shape[1]);
    int ny = static_cast<int>(shape[2]);
    int nx = static_cast<int>(shape[3]);

    size_t expected = static_cast<size_t>(nx)*ny*nz;
    if (vol.size() != expected) {
        std::cerr << "数据大小与shape不一致\n"; return 1;
    }

    if (printStats) {
        double mn = vol[0], mx = vol[0], sum = 0.0;
        for (size_t i=0;i<vol.size();++i) { if (vol[i]<mn) mn=vol[i]; if (vol[i]>mx) mx=vol[i]; sum += vol[i]; }
        double mean = sum / static_cast<double>(vol.size());
        std::cout << "数据统计: min=" << mn << ", max=" << mx << ", mean=" << mean << "\n";
    }

    // Marching Cubes
    std::vector<MCVertex> verts; std::vector<MCTriangle> tris;
    marching_cubes(vol.data(), nx, ny, nz, iso, verts, tris);

    if (!write_vtk_legacy_polydata(outVTK, verts, tris, err)) {
        std::cerr << "写VTK失败: " << err << "\n"; return 1;
    }

    std::cout << "完成。顶点: " << verts.size() << ", 三角形: " << tris.size() << "\n";
    return 0;
}


