#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstring>
#include <chrono>
#include "marching_cubes_hls.h"
#include "npy_reader.h"
#include "vtk_writer.h"

// 打印使用说明
void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <input.npy> <isovalue> <output.vtk> [--with-normals]" << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  input.npy      : Input NPY volume data file" << std::endl;
    std::cout << "  isovalue       : Isovalue for surface extraction" << std::endl;
    std::cout << "  output.vtk     : Output VTK mesh file" << std::endl;
    std::cout << "  --with-normals : (Optional) Output VTK with normals" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << " data.npy 0.5 output.vtk" << std::endl;
    std::cout << "  " << program_name << " data.npy 0.5 output.vtk --with-normals" << std::endl;
}

// 生成球体测试数据
void generate_sphere_data(std::vector<data_t>& volume, int nx, int ny, int nz) {
    float cx = nx / 2.0f;
    float cy = ny / 2.0f;
    float cz = nz / 2.0f;
    float radius = std::min(std::min(nx, ny), nz) / 3.0f;
    
    std::cout << "Generating sphere test data:" << std::endl;
    std::cout << "  Center: (" << cx << ", " << cy << ", " << cz << ")" << std::endl;
    std::cout << "  Radius: " << radius << std::endl;
    
    for (int z = 0; z < nz; z++) {
        for (int y = 0; y < ny; y++) {
            for (int x = 0; x < nx; x++) {
                float dx = x - cx;
                float dy = y - cy;
                float dz = z - cz;
                float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                volume[z * ny * nx + y * nx + x] = radius - dist;
            }
        }
    }
}

// 打印数据统计信息
void print_data_statistics(const std::vector<data_t>& volume) {
    if (volume.empty()) return;
    
    data_t min_val = volume[0];
    data_t max_val = volume[0];
    double sum = 0.0;
    
    for (size_t i = 0; i < volume.size(); i++) {
        data_t val = volume[i];
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
        sum += val;
    }
    
    double mean = sum / volume.size();
    
    std::cout << "Data statistics:" << std::endl;
    std::cout << "  Min value: " << min_val << std::endl;
    std::cout << "  Max value: " << max_val << std::endl;
    std::cout << "  Mean value: " << mean << std::endl;
}

// 验证结果
bool verify_results(const Vertex* vertices, const Triangle* triangles,
                   int num_vertices, int num_triangles) {
    if (num_vertices <= 0 || num_triangles <= 0) {
        std::cerr << "Error: No geometry generated" << std::endl;
        return false;
    }
    
    // 验证三角形索引在有效范围内
    for (int i = 0; i < num_triangles; i++) {
        if (triangles[i].v0 >= (unsigned int)num_vertices ||
            triangles[i].v1 >= (unsigned int)num_vertices ||
            triangles[i].v2 >= (unsigned int)num_vertices) {
            std::cerr << "Error: Triangle " << i << " has invalid indices" << std::endl;
            return false;
        }
    }
    
    // 验证顶点坐标有效
    for (int i = 0; i < num_vertices; i++) {
        if (std::isnan(vertices[i].x) || std::isnan(vertices[i].y) || std::isnan(vertices[i].z) ||
            std::isinf(vertices[i].x) || std::isinf(vertices[i].y) || std::isinf(vertices[i].z)) {
            std::cerr << "Error: Vertex " << i << " has invalid coordinates" << std::endl;
            return false;
        }
    }
    
    std::cout << "Result verification passed!" << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "  Marching Cubes HLS Test (Streaming)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // 解析命令行参数
    std::string input_file;
    std::string output_file;
    data_t isovalue = 0.0f;
    bool with_normals = false;
    bool use_test_data = false;
    
    if (argc < 2) {
        std::cout << "No input file provided, using built-in test data" << std::endl;
        use_test_data = true;
        output_file = "output_test.vtk";
    } else if (argc < 4) {
        print_usage(argv[0]);
        return 1;
    } else {
        input_file = argv[1];
        isovalue = std::atof(argv[2]);
        output_file = argv[3];
        
        // 检查是否需要输出法向量
        for (int i = 4; i < argc; i++) {
            if (std::strcmp(argv[i], "--with-normals") == 0) {
                with_normals = true;
            }
        }
    }
    
    // 加载数据
    std::vector<data_t> volume;
    int nx, ny, nz;
    
    // 根据输入加载或生成数据
    if (use_test_data) {
        // 使用内置球体测试数据
        nx = 32;
        ny = 32;
        nz = 32;
        isovalue = 0.0f;
        
        std::cout << "Using built-in test data:" << std::endl;
        std::cout << "  Dimensions: " << nx << " x " << ny << " x " << nz << std::endl;
        std::cout << "  Isovalue: " << isovalue << std::endl;
        std::cout << std::endl;
        
        volume.resize(nx * ny * nz);
        generate_sphere_data(volume, nx, ny, nz);
    } else {
        // 从NPY文件加载数据
        std::cout << "Loading NPY file: " << input_file << std::endl;
        
        NpyReader reader;
        if (!reader.load(input_file)) {
            std::cerr << "Failed to load NPY file" << std::endl;
            return 1;
        }
        
        volume = reader.getData();
        const auto& shape = reader.getShape();
        
        // 验证数据维度
        if (shape.size() != 4) {
            std::cerr << "Error: Expected 4D data (1, D, H, W), got " << shape.size() << " dimensions" << std::endl;
            return 1;
        }
        
        if (shape[0] != 1) {
            std::cerr << "Error: First dimension must be 1, got " << shape[0] << std::endl;
            return 1;
        }
        
        nz = shape[1];
        ny = shape[2];
        nx = shape[3];
        
        std::cout << "Data dimensions: " << nx << " x " << ny << " x " << nz << std::endl;
        std::cout << "Isovalue: " << isovalue << std::endl;
        std::cout << std::endl;
        
        // 打印数据统计信息
        print_data_statistics(volume);
        std::cout << std::endl;
    }
    
    // 验证尺寸限制
    if (nx > MAX_DIM || ny > MAX_DIM || nz > MAX_DIM) {
        std::cerr << "Error: Data dimensions exceed limit (MAX_DIM=" << MAX_DIM << ")" << std::endl;
        return 1;
    }
    
    // 分配输出缓冲区
    std::cout << "Allocating memory..." << std::endl;
    Vertex* vertices = new Vertex[MAX_VERTICES];
    Triangle* triangles = new Triangle[MAX_TRIANGLES];
    
    if (!vertices || !triangles) {
        std::cerr << "Error: Failed to allocate memory" << std::endl;
        if (vertices) delete[] vertices;
        if (triangles) delete[] triangles;
        return 1;
    }
    
    std::memset(vertices, 0, MAX_VERTICES * sizeof(Vertex));
    std::memset(triangles, 0, MAX_TRIANGLES * sizeof(Triangle));
    
    // 创建stream对象
    hls::stream<Vertex> vertex_stream("vertex_stream");
    hls::stream<Triangle> triangle_stream("triangle_stream");
    
    int num_vertices = 0;
    int num_triangles = 0;
    
    // 运行 Marching Cubes 算法
    std::cout << "Running Marching Cubes algorithm (Streaming)..." << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 调用核心算法
    marching_cubes_hls(
        volume.data(),
        nx, ny, nz,
        isovalue,
        vertex_stream,
        triangle_stream,
        &num_vertices,
        &num_triangles
    );
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Algorithm execution completed!" << std::endl;
    std::cout << "  Execution time: " << duration.count() << " ms" << std::endl;
    std::cout << std::endl;
    
    // 从stream读取结果到数组
    std::cout << "Reading results from streams..." << std::endl;
    
    for (int i = 0; i < num_vertices; i++) {
        if (!vertex_stream.empty()) {
            vertices[i] = vertex_stream.read();
        }
    }
    
    for (int i = 0; i < num_triangles; i++) {
        if (!triangle_stream.empty()) {
            triangles[i] = triangle_stream.read();
        }
    }
    
    // 输出结果统计
    std::cout << "Result statistics:" << std::endl;
    std::cout << "  Generated vertices: " << num_vertices << std::endl;
    std::cout << "  Generated triangles: " << num_triangles << std::endl;
    
    if (num_vertices > 0 && num_triangles > 0) {
        // 计算包围盒
        float min_x = vertices[0].x, max_x = vertices[0].x;
        float min_y = vertices[0].y, max_y = vertices[0].y;
        float min_z = vertices[0].z, max_z = vertices[0].z;
        
        for (int i = 1; i < num_vertices; i++) {
            if (vertices[i].x < min_x) min_x = vertices[i].x;
            if (vertices[i].x > max_x) max_x = vertices[i].x;
            if (vertices[i].y < min_y) min_y = vertices[i].y;
            if (vertices[i].y > max_y) max_y = vertices[i].y;
            if (vertices[i].z < min_z) min_z = vertices[i].z;
            if (vertices[i].z > max_z) max_z = vertices[i].z;
        }
        
        std::cout << "  Bounding box: [" << min_x << ", " << max_x << "] x ["
                  << min_y << ", " << max_y << "] x ["
                  << min_z << ", " << max_z << "]" << std::endl;
    }
    std::cout << std::endl;
    
    // 验证结果
    bool valid = verify_results(vertices, triangles, num_vertices, num_triangles);
    std::cout << std::endl;
    
    // 保存为VTK文件
    if (valid && num_triangles > 0) {
        std::cout << "Saving VTK file: " << output_file << std::endl;
        
        VtkWriter writer;
        bool success;
        
        if (with_normals) {
            success = writer.saveWithNormals(output_file, 
                                            vertices, 
                                            triangles,
                                            num_vertices,
                                            num_triangles);
        } else {
            success = writer.save(output_file, 
                                 vertices, 
                                 triangles,
                                 num_vertices,
                                 num_triangles);
        }
        
        if (!success) {
            std::cerr << "Failed to save VTK file" << std::endl;
        }
    } else {
        std::cout << "Warning: No triangles generated or validation failed, skipping VTK save" << std::endl;
    }
    
    // 释放内存
    delete[] vertices;
    delete[] triangles;
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Test completed!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return valid ? 0 : 1;
}