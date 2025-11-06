#ifndef NPY_READER_H
#define NPY_READER_H

#include <string>
#include <vector>
#include <cstdint>

class NpyReader {
public:
    NpyReader();
    ~NpyReader();
    
    // 加载NPY文件
    bool load(const std::string& filename);
    
    // 获取数据
    const std::vector<float>& getData() const { return data_; }
    
    // 获取形状
    const std::vector<size_t>& getShape() const { return shape_; }
    
    // 获取维度
    size_t getNx() const { return shape_.size() >= 4 ? shape_[3] : 0; }
    size_t getNy() const { return shape_.size() >= 4 ? shape_[2] : 0; }
    size_t getNz() const { return shape_.size() >= 4 ? shape_[1] : 0; }
    
private:
    std::vector<float> data_;
    std::vector<size_t> shape_;
    std::string dtype_;
    bool fortran_order_;
    
    bool parseHeader(std::ifstream& file);
    bool readData(std::ifstream& file);
};

#endif
