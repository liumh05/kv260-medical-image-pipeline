#include "npy_reader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>

static bool parse_npy_header(std::istream &in, NpyInfo &info, size_t &dataOffset, std::string &err) {
    // Magic string: \x93NUMPY
    char magic[6];
    if (!in.read(magic, 6)) { err = "无法读取NPY魔数"; return false; }
    if (!(magic[0] == char(0x93) && magic[1]=='N' && magic[2]=='U' && magic[3]=='M' && magic[4]=='P' && magic[5]=='Y')) {
        err = "不是NPY文件"; return false;
    }
    uint8_t vMajor=0, vMinor=0;
    if (!in.read(reinterpret_cast<char*>(&vMajor), 1)) { err = "无法读取版本"; return false; }
    if (!in.read(reinterpret_cast<char*>(&vMinor), 1)) { err = "无法读取版本"; return false; }

    size_t headerLen = 0;
    if (vMajor == 1) {
        uint16_t len16 = 0;
        if (!in.read(reinterpret_cast<char*>(&len16), 2)) { err = "无法读取头长度(v1)"; return false; }
        headerLen = static_cast<size_t>(len16);
    } else {
        uint32_t len32 = 0;
        if (!in.read(reinterpret_cast<char*>(&len32), 4)) { err = "无法读取头长度(v2/v3)"; return false; }
        headerLen = static_cast<size_t>(len32);
    }

    std::string header(headerLen, '\0');
    if (!in.read(header.data(), headerLen)) { err = "无法读取头内容"; return false; }

    // Robust parse: descr
    auto find_key = [&](const std::string &key)->size_t {
        size_t p = header.find("'"+key+"'");
        if (p == std::string::npos) p = header.find("\""+key+"\"");
        return p;
    };

    auto parse_quoted_value_after_colon = [&](size_t keyPos)->std::string {
        if (keyPos == std::string::npos) return std::string();
        size_t colon = header.find(":", keyPos);
        if (colon == std::string::npos) return std::string();
        size_t i = colon + 1;
        while (i < header.size() && (header[i]==' ')) i++;
        if (i >= header.size()) return std::string();
        if (header[i] != '\'' && header[i] != '"') return std::string();
        char q = header[i++];
        size_t end = header.find(q, i);
        if (end == std::string::npos) return std::string();
        return header.substr(i, end - i);
    };

    auto parse_bool_after_colon = [&](size_t keyPos)->bool {
        if (keyPos == std::string::npos) return false;
        size_t colon = header.find(":", keyPos);
        if (colon == std::string::npos) return false;
        size_t i = colon + 1;
        while (i < header.size() && (header[i]==' ')) i++;
        return header.compare(i, 4, "True") == 0 || header.compare(i, 4, "true") == 0;
    };

    auto parse_shape_tuple = [&](size_t keyPos)->std::vector<size_t> {
        std::vector<size_t> shp;
        if (keyPos == std::string::npos) return shp;
        size_t colon = header.find(":", keyPos);
        if (colon == std::string::npos) return shp;
        size_t l = header.find('(', colon);
        if (l == std::string::npos) return shp;
        size_t r = header.find(')', l+1);
        if (r == std::string::npos) return shp;
        std::string s = header.substr(l+1, r - (l+1));
        size_t i = 0;
        while (i < s.size()) {
            while (i < s.size() && (s[i]==' ' || s[i]==',')) i++;
            size_t j = i;
            while (j < s.size() && isdigit(static_cast<unsigned char>(s[j]))) j++;
            if (j > i) {
                try {
                    shp.push_back(static_cast<size_t>(std::stoll(s.substr(i, j-i))));
                } catch (...) {}
            }
            i = j + 1;
        }
        return shp;
    };

    info.descr = parse_quoted_value_after_colon(find_key("descr"));
    info.fortranOrder = parse_bool_after_colon(find_key("fortran_order"));
    info.shape = parse_shape_tuple(find_key("shape"));

    if (info.shape.empty()) { err = "shape为空"; return false; }
    if (info.fortranOrder) { err = "暂不支持Fortran顺序NPY"; return false; }

    dataOffset = static_cast<size_t>(in.tellg());
    return true;
}

template <typename SrcT>
static void convert_to_float(const SrcT *src, size_t count, std::vector<float> &dst) {
    dst.resize(count);
    for (size_t i=0;i<count;++i) dst[i] = static_cast<float>(src[i]);
}

bool load_npy_to_float(const std::string &path,
                       std::vector<size_t> &shapeOut,
                       std::vector<float> &dataOut,
                       std::string &errorMessage) {
    std::ifstream fin(path, std::ios::binary);
    if (!fin) { errorMessage = "无法打开文件: " + path; return false; }
    NpyInfo info;
    size_t offset=0;
    if (!parse_npy_header(fin, info, offset, errorMessage)) return false;

    // dtype
    size_t elemSize = 0;
    enum class DType { F4, I2, I4, U1 };
    DType dt;
    bool bigEndian = false;
    if (!info.descr.empty() && (info.descr[0] == '>' || info.descr[0] == '!' )) bigEndian = true; // big-endian
    if (info.descr.find("f4") != std::string::npos) { elemSize=4; dt=DType::F4; }
    else if (info.descr.find("i2") != std::string::npos) { elemSize=2; dt=DType::I2; }
    else if (info.descr.find("i4") != std::string::npos) { elemSize=4; dt=DType::I4; }
    else if (info.descr.find("u1") != std::string::npos) { elemSize=1; dt=DType::U1; }
    else { errorMessage = "不支持的dtype: " + info.descr; return false; }

    // total count
    size_t count = 1;
    for (size_t d : info.shape) count *= d;

    // read raw
    fin.seekg(offset, std::ios::beg);
    std::vector<char> raw(count * elemSize);
    if (!fin.read(raw.data(), raw.size())) { errorMessage = "读取数据失败"; return false; }

    // If big-endian, byteswap to little-endian
    if (bigEndian && elemSize > 1) {
        for (size_t i=0; i<raw.size(); i += elemSize) {
            for (size_t a=0, b=elemSize-1; a<b; ++a, --b) {
                std::swap(raw[i+a], raw[i+b]);
            }
        }
    }

    // Convert to float assuming native little-endian host
    dataOut.clear();
    switch (dt) {
        case DType::F4:
            convert_to_float(reinterpret_cast<const float*>(raw.data()), count, dataOut);
            break;
        case DType::I2:
            convert_to_float(reinterpret_cast<const int16_t*>(raw.data()), count, dataOut);
            break;
        case DType::I4:
            convert_to_float(reinterpret_cast<const int32_t*>(raw.data()), count, dataOut);
            break;
        case DType::U1:
            convert_to_float(reinterpret_cast<const uint8_t*>(raw.data()), count, dataOut);
            break;
    }

    shapeOut = info.shape;
    return true;
}


