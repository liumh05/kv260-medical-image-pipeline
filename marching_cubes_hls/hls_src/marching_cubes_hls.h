#ifndef MARCHING_CUBES_HLS_H
#define MARCHING_CUBES_HLS_H

#include <ap_int.h>
#include <hls_stream.h>

// 配置参数
#define MAX_DIM 128
#define MAX_VERTICES 1000000
#define MAX_TRIANGLES 2000000

// 数据类型定义
typedef float data_t;
typedef float vertex_t;
typedef unsigned int index_t;

// 顶点结构体
struct Vertex {
    vertex_t x;
    vertex_t y;
    vertex_t z;
};

// 三角形结构体
struct Triangle {
    index_t v0;
    index_t v1;
    index_t v2;
};

// 立方体顶点值
struct CubeValues {
    data_t v[8];
};

// 主函数 - Streaming版本
void marching_cubes_hls(
    const data_t* volume,
    int nx, int ny, int nz,
    data_t isovalue,
    hls::stream<Vertex>& vertex_stream,
    hls::stream<Triangle>& triangle_stream,
    int* num_vertices,
    int* num_triangles
);

// 辅助函数：Stream转Memory（用于输出）
void stream_to_memory_vertices(
    hls::stream<Vertex>& stream,
    Vertex* mem,
    int count
);

void stream_to_memory_triangles(
    hls::stream<Triangle>& stream,
    Triangle* mem,
    int count
);

#endif