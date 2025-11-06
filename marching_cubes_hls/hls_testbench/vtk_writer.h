#ifndef VTK_WRITER_H
#define VTK_WRITER_H

#include <string>
#include <vector>
#include "marching_cubes_hls.h"

class VtkWriter {
public:
    VtkWriter();
    ~VtkWriter();
    
    // 保存VTK文件
    bool save(const std::string& filename,
              const Vertex* vertices,
              const Triangle* triangles,
              int num_vertices,
              int num_triangles);
    
    // 保存带法向量的VTK文件
    bool saveWithNormals(const std::string& filename,
                        const Vertex* vertices,
                        const Triangle* triangles,
                        int num_vertices,
                        int num_triangles);
};

#endif
