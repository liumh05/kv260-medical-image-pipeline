#include "npy_reader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>

NpyReader::NpyReader() : fortran_order_(false) {}

NpyReader::~NpyReader() {}

bool NpyReader::load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return false;
    }
    
    if (!parseHeader(file)) {
        std::cerr << "解析NPY头失败" << std::endl;
        return false;
    }
    
    if (!readData(file)) {
        std::cerr << "读取NPY数据失败" << std::endl;
        return false;
    }
    
    file.close();
    return true;
}

bool NpyReader::parseHeader(std::ifstream& file) {
    // 读取magic number
    char magic[6];
    file.read(magic, 6);
    if (std::string(magic, 6) != "\x93NUMPY") {
        std::cerr << "不是有效的NPY文件" << std::endl;
        return false;
    }
    
    // 读取版本
    uint8_t major, minor;
    file.read(reinterpret_cast<char*>(&major), 1);
    file.read(reinterpret_cast<char*>(&minor), 1);
    
    // 读取header长度
    uint16_t header_len;
    if (major == 1) {
        file.read(reinterpret_cast<char*>(&header_len), 2);
    } else {
        uint32_t header_len_32;
        file.read(reinterpret_cast<char*>(&header_len_32), 4);
        header_len = static_cast<uint16_t>(header_len_32);
    }
    
    // 读取header字符串
    std::vector<char> header_str(header_len);
    file.read(header_str.data(), header_len);
    std::string header(header_str.begin(), header_str.end());
    
    // 解析shape
    size_t shape_start = header.find("'shape': (");
    if (shape_start == std::string::npos) {
        shape_start = header.find("\"shape\": (");
    }
    if (shape_start != std::string::npos) {
        size_t shape_end = header.find(")", shape_start);
        std::string shape_str = header.substr(shape_start + 10, shape_end - shape_start - 10);
        
        std::stringstream ss(shape_str);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item.erase(std::remove_if(item.begin(), item.end(), ::isspace), item.end());
            if (!item.empty()) {
                shape_.push_back(std::stoull(item));
            }
        }
    }
    
    // 解析dtype
    size_t dtype_start = header.find("'descr': '");
    if (dtype_start == std::string::npos) {
        dtype_start = header.find("\"descr\": \"");
    }
    if (dtype_start != std::string::npos) {
        size_t dtype_end = header.find("'", dtype_start + 10);
        if (dtype_end == std::string::npos) {
            dtype_end = header.find("\"", dtype_start + 10);
        }
        dtype_ = header.substr(dtype_start + 10, dtype_end - dtype_start - 10);
    }
    
    // 解析fortran_order
    fortran_order_ = header.find("'fortran_order': True") != std::string::npos ||
                     header.find("\"fortran_order\": True") != std::string::npos;
    
    std::cout << "NPY文件信息:" << std::endl;
    std::cout << "  Shape: (";
    for (size_t i = 0; i < shape_.size(); i++) {
        std::cout << shape_[i];
        if (i < shape_.size() - 1) std::cout << ", ";
    }
    std::cout << ")" << std::endl;
    std::cout << "  Dtype: " << dtype_ << std::endl;
    std::cout << "  Fortran order: " << (fortran_order_ ? "True" : "False") << std::endl;
    
    return true;
}

bool NpyReader::readData(std::ifstream& file) {
    // 计算总元素数
    size_t total_elements = 1;
    for (size_t dim : shape_) {
        total_elements *= dim;
    }
    
    data_.resize(total_elements);
    
    // 根据dtype读取数据
    if (dtype_ == "<f4" || dtype_ == ">f4" || dtype_ == "=f4") {
        // float32
        file.read(reinterpret_cast<char*>(data_.data()), total_elements * sizeof(float));
    } else if (dtype_ == "<f8" || dtype_ == ">f8" || dtype_ == "=f8") {
        // float64
        std::vector<double> temp(total_elements);
        file.read(reinterpret_cast<char*>(temp.data()), total_elements * sizeof(double));
        for (size_t i = 0; i < total_elements; i++) {
            data_[i] = static_cast<float>(temp[i]);
        }
    } else if (dtype_ == "<i2" || dtype_ == ">i2" || dtype_ == "=i2") {
        // int16
        std::vector<int16_t> temp(total_elements);
        file.read(reinterpret_cast<char*>(temp.data()), total_elements * sizeof(int16_t));
        for (size_t i = 0; i < total_elements; i++) {
            data_[i] = static_cast<float>(temp[i]);
        }
    } else if (dtype_ == "<i4" || dtype_ == ">i4" || dtype_ == "=i4") {
        // int32
        std::vector<int32_t> temp(total_elements);
        file.read(reinterpret_cast<char*>(temp.data()), total_elements * sizeof(int32_t));
        for (size_t i = 0; i < total_elements; i++) {
            data_[i] = static_cast<float>(temp[i]);
        }
    } else if (dtype_ == "|u1" || dtype_ == "u1") {
        // uint8
        std::vector<uint8_t> temp(total_elements);
        file.read(reinterpret_cast<char*>(temp.data()), total_elements * sizeof(uint8_t));
        for (size_t i = 0; i < total_elements; i++) {
            data_[i] = static_cast<float>(temp[i]);
        }
    } else {
        std::cerr << "不支持的数据类型: " << dtype_ << std::endl;
        return false;
    }
    
    std::cout << "成功读取 " << total_elements << " 个元素" << std::endl;
    return true;
}
