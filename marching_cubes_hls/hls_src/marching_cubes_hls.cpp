#include "marching_cubes_hls.h"
#include "mc_tables.h"
#include <hls_math.h>

// 线性插值
inline data_t interpolate(data_t v0, data_t v1, data_t iso) {
    #pragma HLS INLINE
    data_t diff = v1 - v0;
    if (hls::fabs(diff) < 1e-6f) {
        return 0.5f;
    }
    return (iso - v0) / diff;
}

// 计算立方体配置索引
inline int get_cube_index(const CubeValues& cube, data_t iso) {
    #pragma HLS INLINE
    int cube_index = 0;
    if (cube.v[0] < iso) cube_index |= 1;
    if (cube.v[1] < iso) cube_index |= 2;
    if (cube.v[2] < iso) cube_index |= 4;
    if (cube.v[3] < iso) cube_index |= 8;
    if (cube.v[4] < iso) cube_index |= 16;
    if (cube.v[5] < iso) cube_index |= 32;
    if (cube.v[6] < iso) cube_index |= 64;
    if (cube.v[7] < iso) cube_index |= 128;
    return cube_index;
}

// 从BRAM缓存获取立方体8个顶点的值
inline void get_cube_values_from_cache(const data_t cache[2][128][128], 
                                       int x, int y, int z_offset,
                                       CubeValues& cube) {
    #pragma HLS INLINE
    
    int z_buf = z_offset & 1;
    int z_next = (z_offset + 1) & 1;
    
    cube.v[0] = cache[z_buf][y][x];
    cube.v[1] = cache[z_buf][y][x + 1];
    cube.v[2] = cache[z_buf][y + 1][x + 1];
    cube.v[3] = cache[z_buf][y + 1][x];
    cube.v[4] = cache[z_next][y][x];
    cube.v[5] = cache[z_next][y][x + 1];
    cube.v[6] = cache[z_next][y + 1][x + 1];
    cube.v[7] = cache[z_next][y + 1][x];
}

// 计算边上的顶点
Vertex compute_edge_vertex(int x, int y, int z, int edge, 
                          const CubeValues& cube, data_t iso) {
    #pragma HLS INLINE
    
    // 标准MC边定义
    const int edge_v0[12] = {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3};
    const int edge_v1[12] = {1, 2, 3, 0, 5, 6, 7, 4, 4, 5, 6, 7};
    
    #pragma HLS ARRAY_PARTITION variable=edge_v0 complete
    #pragma HLS ARRAY_PARTITION variable=edge_v1 complete
    
    // 立方体8个顶点的相对坐标
    const float vx[8] = {0, 1, 1, 0, 0, 1, 1, 0};
    const float vy[8] = {0, 0, 1, 1, 0, 0, 1, 1};
    const float vz[8] = {0, 0, 0, 0, 1, 1, 1, 1};
    
    #pragma HLS ARRAY_PARTITION variable=vx complete
    #pragma HLS ARRAY_PARTITION variable=vy complete
    #pragma HLS ARRAY_PARTITION variable=vz complete
    
    int v0 = edge_v0[edge];
    int v1 = edge_v1[edge];
    
    data_t t = interpolate(cube.v[v0], cube.v[v1], iso);
    
    Vertex vertex;
    vertex.x = x + vx[v0] + t * (vx[v1] - vx[v0]);
    vertex.y = y + vy[v0] + t * (vy[v1] - vy[v0]);
    vertex.z = z + vz[v0] + t * (vz[v1] - vz[v0]);
    
    return vertex;
}

// 加载一个z平面到缓存
void load_z_plane(const data_t* volume, int nx, int ny, int nz, int z,
                  data_t cache[128][128]) {
    #pragma HLS INLINE off
    
    // *** 修改：使用 MAX_DIM (128) 限制加载边界 ***
    if (z >= nz || z >= MAX_DIM) return;
    
    int plane_offset = z * ny * nx;
    int y_max = (ny < MAX_DIM) ? ny : MAX_DIM;
    int x_max = (nx < MAX_DIM) ? nx : MAX_DIM;
    
    LOAD_Y: for (int y = 0; y < y_max; y++) {
        #pragma HLS loop_tripcount min=64 max=128 avg=128
        LOAD_X: for (int x = 0; x < x_max; x++) {
            #pragma HLS PIPELINE II=1
            #pragma HLS loop_tripcount min=64 max=128 avg=128
            cache[y][x] = volume[plane_offset + y * nx + x];
        }
    }
}

// 主函数 - Streaming版本
void marching_cubes_hls(
    const data_t* volume,
    int nx, int ny, int nz,
    data_t isovalue,
    hls::stream<Vertex>& vertex_stream,
    hls::stream<Triangle>& triangle_stream,
    int* num_vertices,
    int* num_triangles
) {
    #pragma HLS INTERFACE m_axi port=volume offset=slave bundle=gmem0 depth=2097152 max_read_burst_length=256
    #pragma HLS INTERFACE axis port=vertex_stream
    #pragma HLS INTERFACE axis port=triangle_stream
    #pragma HLS INTERFACE s_axilite port=nx
    #pragma HLS INTERFACE s_axilite port=ny
    #pragma HLS INTERFACE s_axilite port=nz
    #pragma HLS INTERFACE s_axilite port=isovalue
    #pragma HLS INTERFACE s_axilite port=num_vertices
    #pragma HLS INTERFACE s_axilite port=num_triangles
    #pragma HLS INTERFACE s_axilite port=return
    
    // BRAM缓存：双缓冲存储两个z平面
    data_t z_plane_cache[2][128][128];
    #pragma HLS BIND_STORAGE variable=z_plane_cache type=RAM_T2P impl=BRAM
    #pragma HLS ARRAY_PARTITION variable=z_plane_cache complete dim=1
    
    // triTable索引到edgeTable位号的映射
    static const int edge_index_map[12] = {0,1,2,3,4,5,6,7,8,9,11,10};
    #pragma HLS ARRAY_PARTITION variable=edge_index_map complete
    
    int v_count = 0;
    int t_count = 0;

    // *** 修改：(BUG 修复) 确定实际的处理边界 ***
    // *** 确保循环不会超过缓存大小 (MAX_DIM) ***
    int const x_bound = (nx > MAX_DIM) ? MAX_DIM : nx;
    int const y_bound = (ny > MAX_DIM) ? MAX_DIM : ny;
    int const z_bound = (nz > MAX_DIM) ? MAX_DIM : nz;
    
    // 预加载第一个z平面
    load_z_plane(volume, nx, ny, nz, 0, z_plane_cache[0]);
    
    // 遍历所有立方体
    // *** 修改：使用 z_bound ***
    Z_LOOP: for (int z = 0; z < z_bound - 1; z++) {
        #pragma HLS loop_tripcount min=1 max=127 avg=64
        
        // 加载下一个z平面到另一个缓冲区
        int next_z = z + 1;
        int cache_idx = next_z & 1;
        load_z_plane(volume, nx, ny, nz, next_z, z_plane_cache[cache_idx]);
        
        // *** 修改：使用 y_bound ***
        Y_LOOP: for (int y = 0; y < y_bound - 1; y++) {
            #pragma HLS loop_tripcount min=1 max=127 avg=64
            // *** 修改：使用 x_bound ***
            X_LOOP: for (int x = 0; x < x_bound - 1; x++) {
                // *** 注意：II=12 仍然是一个潜在瓶颈，但 UNROLL 应该会有所帮助 ***
                #pragma HLS PIPELINE II=12
                #pragma HLS loop_tripcount min=1 max=127 avg=64
                
                // 从缓存获取立方体值
                CubeValues cube;
                #pragma HLS ARRAY_PARTITION variable=cube.v complete
                get_cube_values_from_cache(z_plane_cache, x, y, z, cube);
                
                // 使用微小负偏置避免浮点精度导致的裂缝
                data_t isoBias = isovalue - 1e-6f;
                int cube_index = get_cube_index(cube, isoBias);
                
                if (cube_index == 0 || cube_index == 255) continue;
                
                int edges = edgeTable[cube_index];
                if (edges == 0) continue;
                
                // 按物理边号存储顶点
                Vertex local_vertices[12];
                #pragma HLS ARRAY_PARTITION variable=local_vertices complete
                
                int vert_map[12];
                #pragma HLS ARRAY_PARTITION variable=vert_map complete
                
                int local_v_count = 0;
                
                // 创建顶点
                EDGE_CREATE: for (int edge = 0; edge < 12; edge++) {
                    #pragma HLS UNROLL
                    if (edges & (1 << edge)) {
                        local_vertices[edge] = compute_edge_vertex(x, y, z, edge, cube, isovalue);
                        vert_map[edge] = v_count + local_v_count;
                        local_v_count++;
                    } else {
                        vert_map[edge] = -1;
                    }
                }
                
                // 写入顶点到stream（只写入有效顶点）
                WRITE_VERTICES: for (int edge = 0; edge < 12; edge++) {
                    #pragma HLS UNROLL
                    if (vert_map[edge] >= 0) {
                        vertex_stream.write(local_vertices[edge]);
                    }
                }
                
                v_count += local_v_count;
                
                // 构建triTable索引视图
                int triEdgeVert[12];
                #pragma HLS ARRAY_PARTITION variable=triEdgeVert complete
                
                MAP_EDGE: for (int ei = 0; ei < 12; ei++) {
                    #pragma HLS UNROLL
                    int bit_idx = edge_index_map[ei];
                    triEdgeVert[ei] = (edges & (1 << bit_idx)) ? vert_map[bit_idx] : -1;
                }
                
                // 生成三角形
                TRI_GEN: for (int i = 0; i < 15; i += 3) {
                    // *** 修改：(性能优化) 展开此循环 ***
                    #pragma HLS UNROLL
                    #pragma HLS loop_tripcount min=0 max=5 avg=2
                    
                    int ia = triTable[cube_index][i];
                    if (ia == -1) break;
                    
                    int ib = triTable[cube_index][i + 1];
                    int ic = triTable[cube_index][i + 2];
                    
                    // 验证索引有效性
                    bool valid = (ib != -1) && (ic != -1) &&
                                (ia >= 0 && ia <= 11) &&
                                (ib >= 0 && ib <= 11) &&
                                (ic >= 0 && ic <= 11) &&
                                (triEdgeVert[ia] >= 0) &&
                                (triEdgeVert[ib] >= 0) &&
                                (triEdgeVert[ic] >= 0);
                    
                    if (valid) {
                        Triangle tri;
                        tri.v0 = triEdgeVert[ia];
                        tri.v1 = triEdgeVert[ib];
                        tri.v2 = triEdgeVert[ic];
                        triangle_stream.write(tri);
                        t_count++;
                    }
                }
            }
        }
    }
    
    *num_vertices = v_count;
    *num_triangles = t_count;
}

// Stream转Memory辅助函数（用于testbench）
void stream_to_memory_vertices(
    hls::stream<Vertex>& stream,
    Vertex* mem,
    int count
) {
    #pragma HLS INLINE off
    
    for (int i = 0; i < count; i++) {
        #pragma HLS PIPELINE II=1
        mem[i] = stream.read();
    }
}

void stream_to_memory_triangles(
    hls::stream<Triangle>& stream,
    Triangle* mem,
    int count
) {
    #pragma HLS INLINE off
    
    for (int i = 0; i < count; i++) {
        #pragma HLS PIPELINE II=1
        mem[i] = stream.read();
    }
}