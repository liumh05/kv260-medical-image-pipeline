#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Minimal NPY loader supporting C-order arrays and dtypes: float32, int16, int32, uint8
// Converts data to float32 output for marching cubes.

struct NpyInfo {
    std::vector<size_t> shape; // e.g., {1, Z, Y, X}
    bool fortranOrder = false;
    std::string descr; // like "<f4", "<i2", "<i4", "|u1"
};

bool load_npy_to_float(const std::string &path,
                       std::vector<size_t> &shapeOut,
                       std::vector<float> &dataOut,
                       std::string &errorMessage);


