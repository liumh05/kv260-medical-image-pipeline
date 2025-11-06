#include "vtk_writer.h"
#include <fstream>
#include <iostream>
#include <cmath>

VtkWriter::VtkWriter() {}

VtkWriter::~VtkWriter() {}

bool VtkWriter::save(const std::string& filename,
                     const Vertex* vertices,
                     const Triangle* triangles,
                     int num_vertices,
                     int num_triangles) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "无法创建文件: " << filename << std::endl;
        return false;
    }
    
    // 写入VTK头
    file << "# vtk DataFile Version 3.0\n";
    file << "Marching Cubes HLS Output\n";
    file << "ASCII\n";
    file << "DATASET POLYDATA\n";
    
    // 写入顶点
    file << "POINTS " << num_vertices << " float\n";
    for (int i = 0; i < num_vertices; i++) {
        file << vertices[i].x << " " 
             << vertices[i].y << " " 
             << vertices[i].z << "\n";
    }
    
    // 写入三角形
    file << "\nPOLYGONS " << num_triangles << " " << (num_triangles * 4) << "\n";
    for (int i = 0; i < num_triangles; i++) {
        file << "3 " 
             << triangles[i].v0 << " " 
             << triangles[i].v1 << " " 
             << triangles[i].v2 << "\n";
    }
    
    file.close();
    std::cout << "VTK文件已保存: " << filename << std::endl;
    std::cout << "  顶点数: " << num_vertices << std::endl;
    std::cout << "  三角形数: " << num_triangles << std::endl;
    
    return true;
}

bool VtkWriter::saveWithNormals(const std::string& filename,
                                const Vertex* vertices,
                                const Triangle* triangles,
                                int num_vertices,
                                int num_triangles) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "无法创建文件: " << filename << std::endl;
        return false;
    }
    
    // 计算法向量
    std::vector<float> normals(num_vertices * 3, 0.0f);
    std::vector<int> vertex_count(num_vertices, 0);
    
    for (int i = 0; i < num_triangles; i++) {
        int v0 = triangles[i].v0;
        int v1 = triangles[i].v1;
        int v2 = triangles[i].v2;
        
        // 计算三角形法向量
        float e1x = vertices[v1].x - vertices[v0].x;
        float e1y = vertices[v1].y - vertices[v0].y;
        float e1z = vertices[v1].z - vertices[v0].z;
        
        float e2x = vertices[v2].x - vertices[v0].x;
        float e2y = vertices[v2].y - vertices[v0].y;
        float e2z = vertices[v2].z - vertices[v0].z;
        
        float nx = e1y * e2z - e1z * e2y;
        float ny = e1z * e2x - e1x * e2z;
        float nz = e1x * e2y - e1y * e2x;
        
        // 归一化
        float len = std::sqrt(nx*nx + ny*ny + nz*nz);
        if (len > 1e-6f) {
            nx /= len;
            ny /= len;
            nz /= len;
        }
        
        // 累加到顶点法向量
        normals[v0*3 + 0] += nx;
        normals[v0*3 + 1] += ny;
        normals[v0*3 + 2] += nz;
        vertex_count[v0]++;
        normals[v1*3 + 0] += nx;
        normals[v1*3 + 1] += ny;
        normals[v1*3 + 2] += nz;
        vertex_count[v1]++;
        
        normals[v2*3 + 0] += nx;
        normals[v2*3 + 1] += ny;
        normals[v2*3 + 2] += nz;
        vertex_count[v2]++;
    }
    
    // 平均并归一化法向量
    for (int i = 0; i < num_vertices; i++) {
        if (vertex_count[i] > 0) {
            normals[i*3 + 0] /= vertex_count[i];
            normals[i*3 + 1] /= vertex_count[i];
            normals[i*3 + 2] /= vertex_count[i];
            
            float len = std::sqrt(normals[i*3 + 0] * normals[i*3 + 0] +
                                 normals[i*3 + 1] * normals[i*3 + 1] +
                                 normals[i*3 + 2] * normals[i*3 + 2]);
            if (len > 1e-6f) {
                normals[i*3 + 0] /= len;
                normals[i*3 + 1] /= len;
                normals[i*3 + 2] /= len;
            }
        }
    }
    
    // 写入VTK头
    file << "# vtk DataFile Version 3.0\n";
    file << "Marching Cubes HLS Output with Normals\n";
    file << "ASCII\n";
    file << "DATASET POLYDATA\n";
    
    // 写入顶点
    file << "POINTS " << num_vertices << " float\n";
    for (int i = 0; i < num_vertices; i++) {
        file << vertices[i].x << " " 
             << vertices[i].y << " " 
             << vertices[i].z << "\n";
    }
    
    // 写入三角形
    file << "\nPOLYGONS " << num_triangles << " " << (num_triangles * 4) << "\n";
    for (int i = 0; i < num_triangles; i++) {
        file << "3 " 
             << triangles[i].v0 << " " 
             << triangles[i].v1 << " " 
             << triangles[i].v2 << "\n";
    }
    
    // 写入法向量
    file << "\nPOINT_DATA " << num_vertices << "\n";
    file << "NORMALS Normals float\n";
    for (int i = 0; i < num_vertices; i++) {
        file << normals[i*3 + 0] << " "
             << normals[i*3 + 1] << " "
             << normals[i*3 + 2] << "\n";
    }
    
    file.close();
    std::cout << "VTK文件（带法向量）已保存: " << filename << std::endl;
    std::cout << "  顶点数: " << num_vertices << std::endl;
    std::cout << "  三角形数: " << num_triangles << std::endl;
    
    return true;
}
