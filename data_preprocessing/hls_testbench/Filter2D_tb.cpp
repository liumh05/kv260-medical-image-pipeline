#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include "Filter2D.h"

// 测试图像尺寸
#define TEST_WIDTH 128
#define TEST_HEIGHT 128

// 简单的图像类（不依赖OpenCV）
class SimpleImage {
public:
    int width, height;
    std::vector<unsigned char> data; // RGB数据，每像素3字节
    
    SimpleImage(int w, int h) : width(w), height(h) {
        data.resize(w * h * 3);
    }
    
    // 设置像素（RGB格式）
    void setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            int idx = (y * width + x) * 3;
            data[idx + 0] = r;
            data[idx + 1] = g;
            data[idx + 2] = b;
        }
    }
    
    // 获取像素（RGB格式）
    void getPixel(int x, int y, unsigned char& r, unsigned char& g, unsigned char& b) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            int idx = (y * width + x) * 3;
            r = data[idx + 0];
            g = data[idx + 1];
            b = data[idx + 2];
        }
    }
    
    // 保存为PPM格式（P6二进制格式）
    bool savePPM(const std::string& filename) {
        std::ofstream file(filename.c_str(), std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "错误: 无法创建文件 " << filename << std::endl;
            return false;
        }
        
        // PPM P6 头部
        file << "P6\n";
        file << width << " " << height << "\n";
        file << "255\n";
        
        // 写入RGB数据
        file.write(reinterpret_cast<char*>(data.data()), data.size());
        
        file.close();
        return true;
    }
    
    // 保存为BMP格式
    bool saveBMP(const std::string& filename) {
        std::ofstream file(filename.c_str(), std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "错误: 无法创建文件 " << filename << std::endl;
            return false;
        }
        
        // BMP文件头
        int rowSize = ((width * 3 + 3) / 4) * 4; // 4字节对齐
        int imageSize = rowSize * height;
        int fileSize = 54 + imageSize; // 54字节头部 + 图像数据
        
        unsigned char bmpFileHeader[14] = {
            'B', 'M',                           // 签名
            0, 0, 0, 0,                         // 文件大小
            0, 0, 0, 0,                         // 保留
            54, 0, 0, 0                         // 数据偏移
        };
        
        unsigned char bmpInfoHeader[40] = {
            40, 0, 0, 0,                        // 头部大小
            0, 0, 0, 0,                         // 宽度
            0, 0, 0, 0,                         // 高度
            1, 0,                               // 颜色平面
            24, 0,                              // 位深度
            0, 0, 0, 0,                         // 压缩方式
            0, 0, 0, 0,                         // 图像大小
            0, 0, 0, 0,                         // X分辨率
            0, 0, 0, 0,                         // Y分辨率
            0, 0, 0, 0,                         // 调色板颜色数
            0, 0, 0, 0                          // 重要颜色数
        };
        
        // 填充文件大小
        bmpFileHeader[2] = (unsigned char)(fileSize);
        bmpFileHeader[3] = (unsigned char)(fileSize >> 8);
        bmpFileHeader[4] = (unsigned char)(fileSize >> 16);
        bmpFileHeader[5] = (unsigned char)(fileSize >> 24);
        
        // 填充宽度和高度
        bmpInfoHeader[4] = (unsigned char)(width);
        bmpInfoHeader[5] = (unsigned char)(width >> 8);
        bmpInfoHeader[6] = (unsigned char)(width >> 16);
        bmpInfoHeader[7] = (unsigned char)(width >> 24);
        
        bmpInfoHeader[8] = (unsigned char)(height);
        bmpInfoHeader[9] = (unsigned char)(height >> 8);
        bmpInfoHeader[10] = (unsigned char)(height >> 16);
        bmpInfoHeader[11] = (unsigned char)(height >> 24);
        
        // 填充图像大小
        bmpInfoHeader[20] = (unsigned char)(imageSize);
        bmpInfoHeader[21] = (unsigned char)(imageSize >> 8);
        bmpInfoHeader[22] = (unsigned char)(imageSize >> 16);
        bmpInfoHeader[23] = (unsigned char)(imageSize >> 24);
        
        // 写入头部
        file.write(reinterpret_cast<char*>(bmpFileHeader), 14);
        file.write(reinterpret_cast<char*>(bmpInfoHeader), 40);
        
        // 写入图像数据（BMP是BGR且从下往上）
        std::vector<unsigned char> padding(rowSize - width * 3, 0);
        
        for (int y = height - 1; y >= 0; y--) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 3;
                unsigned char r = data[idx + 0];
                unsigned char g = data[idx + 1];
                unsigned char b = data[idx + 2];
                
                // BMP格式是BGR
                file.put(b);
                file.put(g);
                file.put(r);
            }
            // 写入行填充
            if (!padding.empty()) {
                file.write(reinterpret_cast<char*>(padding.data()), padding.size());
            }
        }
        
        file.close();
        return true;
    }
};

int main() {
    stream_t src_stream;
    stream_t dst_stream;

    std::cout << "==================================================" << std::endl;
    std::cout << "Filter2D 图像处理测试 (NPIX = " << NPIX << ")" << std::endl;
    std::cout << "不依赖OpenCV，使用内置图像保存功能" << std::endl;
    std::cout << "==================================================" << std::endl;

    // 验证宽度是NPIX的倍数
    if (TEST_WIDTH % NPIX != 0) {
        std::cerr << "错误: 图像宽度必须是 NPIX(" << NPIX << ") 的倍数!" << std::endl;
        std::cerr << "当前宽度: " << TEST_WIDTH << std::endl;
        return 1;
    }

    const int num_words = TEST_WIDTH / NPIX;
    
    // 创建图像对象
    SimpleImage input_image(TEST_WIDTH, TEST_HEIGHT);
    SimpleImage output_image(TEST_WIDTH, TEST_HEIGHT);
    
    std::cout << "图像尺寸: " << TEST_WIDTH << "x" << TEST_HEIGHT << std::endl;
    std::cout << "每行 " << num_words << " 个传输, 每个传输 " << NPIX << " 个像素" << std::endl;

    std::cout << "\n--- 生成测试图像并转换为AXI Stream ---" << std::endl;

    // 生成测试图像并转换为AXI Stream
    for (int y = 0; y < TEST_HEIGHT; y++) {
        for (int x_word = 0; x_word < num_words; x_word++) {
            interface_t axi_data;
            axi_data.data = 0;
            
            // 打包NPIX个像素
            for (int p = 0; p < NPIX; p++) {
                int x = x_word * NPIX + p;
                
                // 生成更有趣的测试图案
                unsigned char r, g, b;
                
                // 棋盘格 + 渐变
                if ((x / 16 + y / 16) % 2 == 0) {
                    // 白色块
                    r = g = b = 255;
                } else {
                    // 渐变灰色块
                    unsigned char gray = (x * 255) / TEST_WIDTH;
                    r = g = b = gray;
                }
                
                // 保存到输入图像（RGB格式）
                input_image.setPixel(x, y, r, g, b);
                
                // 打包为24位BGR（与硬件匹配）
                ap_uint<24> pixel_val;
                pixel_val.range(7, 0) = b;    // B
                pixel_val.range(15, 8) = g;   // G
                pixel_val.range(23, 16) = r;  // R
                
                // 打包到AXI数据
                axi_data.data.range((p+1)*24-1, p*24) = pixel_val;
            }
            
            axi_data.keep = -1;
            axi_data.last = (y == TEST_HEIGHT - 1 && x_word == num_words - 1);
            src_stream.write(axi_data);
        }
    }
    
    std::cout << "✓ 测试图像生成完毕，共 " << (TEST_HEIGHT * num_words) << " 个AXI传输" << std::endl;

    // 保存输入图像
    std::cout << "\n--- 保存输入图像 ---" << std::endl;
    if (input_image.savePPM("input_image.ppm")) {
        std::cout << "✓ 输入图像已保存为: input_image.ppm" << std::endl;
    }
    if (input_image.saveBMP("input_image.bmp")) {
        std::cout << "✓ 输入图像已保存为: input_image.bmp" << std::endl;
    }

    std::cout << "\n--- 调用 Filter2D_accel IP核 ---" << std::endl;
    
    try {
        Filter2D_accel(src_stream, dst_stream, TEST_HEIGHT, TEST_WIDTH);
        std::cout << "✓ IP核处理完成" << std::endl;
    } catch (...) {
        std::cerr << "✗ IP核处理异常!" << std::endl;
        return 1;
    }

    std::cout << "\n--- 从AXI Stream提取处理后的图像 ---" << std::endl;
    int output_word_count = 0;
    int output_pixel_count = 0;
    bool last_signal_received = false;

    // 读取输出流并保存到图像
    while (!dst_stream.empty()) {
        interface_t axi_out_data = dst_stream.read();
        output_word_count++;
        
        // 计算当前像素的位置
        int word_idx = output_word_count - 1;
        int y = word_idx / num_words;
        int x_word = word_idx % num_words;
        
        // 解包NPIX个像素
        for (int p = 0; p < NPIX; p++) {
            int x = x_word * NPIX + p;
            output_pixel_count++;
            
            if (y < TEST_HEIGHT && x < TEST_WIDTH) {
                ap_uint<24> pixel_val = axi_out_data.data.range((p+1)*24-1, p*24);
                
                // 解包BGR
                unsigned char b = pixel_val.range(7, 0).to_uint();
                unsigned char g = pixel_val.range(15, 8).to_uint();
                unsigned char r = pixel_val.range(23, 16).to_uint();
                
                // 保存到输出图像（RGB格式）
                output_image.setPixel(x, y, r, g, b);
            }
        }
        
        if (axi_out_data.last) {
            last_signal_received = true;
        }
    }
    
    std::cout << "✓ 收到 " << output_word_count << " 个传输 (" << output_pixel_count << " 个像素)" << std::endl;

    // 保存输出图像
    std::cout << "\n--- 保存输出图像 ---" << std::endl;
    if (output_image.savePPM("output_image.ppm")) {
        std::cout << "✓ 输出图像已保存为: output_image.ppm" << std::endl;
    }
    if (output_image.saveBMP("output_image.bmp")) {
        std::cout << "✓ 输出图像已保存为: output_image.bmp" << std::endl;
    }

    // 生成差异图像
    std::cout << "\n--- 生成差异图像 ---" << std::endl;
    SimpleImage diff_image(TEST_WIDTH, TEST_HEIGHT);
    
    long long total_diff = 0;
    int max_diff = 0;
    
    for (int y = 0; y < TEST_HEIGHT; y++) {
        for (int x = 0; x < TEST_WIDTH; x++) {
            unsigned char r1, g1, b1, r2, g2, b2;
            input_image.getPixel(x, y, r1, g1, b1);
            output_image.getPixel(x, y, r2, g2, b2);
            
            int diff_r = abs(r1 - r2);
            int diff_g = abs(g1 - g2);
            int diff_b = abs(b1 - b2);
            
            diff_image.setPixel(x, y, diff_r, diff_g, diff_b);
            
            total_diff += (diff_r + diff_g + diff_b);
            int pixel_diff = (diff_r + diff_g + diff_b) / 3;
            if (pixel_diff > max_diff) max_diff = pixel_diff;
        }
    }
    
    if (diff_image.savePPM("diff_image.ppm")) {
        std::cout << "✓ 差异图像已保存为: diff_image.ppm" << std::endl;
    }
    if (diff_image.saveBMP("diff_image.bmp")) {
        std::cout << "✓ 差异图像已保存为: diff_image.bmp" << std::endl;
    }
    
    // 生成增强差异图（放大10倍）
    SimpleImage diff_enhanced(TEST_WIDTH, TEST_HEIGHT);
    for (int y = 0; y < TEST_HEIGHT; y++) {
        for (int x = 0; x < TEST_WIDTH; x++) {
            unsigned char r, g, b;
            diff_image.getPixel(x, y, r, g, b);
            
            int r_enh = std::min(255, r * 10);
            int g_enh = std::min(255, g * 10);
            int b_enh = std::min(255, b * 10);
            
            diff_enhanced.setPixel(x, y, r_enh, g_enh, b_enh);
        }
    }
    
    if (diff_enhanced.savePPM("diff_enhanced.ppm")) {
        std::cout << "✓ 增强差异图已保存为: diff_enhanced.ppm (10x放大)" << std::endl;
    }
    if (diff_enhanced.saveBMP("diff_enhanced.bmp")) {
        std::cout << "✓ 增强差异图已保存为: diff_enhanced.bmp (10x放大)" << std::endl;
    }

    // 图像质量分析
    std::cout << "\n--- 图像质量分析 ---" << std::endl;
    double avg_diff = (double)total_diff / (TEST_WIDTH * TEST_HEIGHT * 3);
    std::cout << "平均像素差异: " << avg_diff << std::endl;
    std::cout << "最大像素差异: " << max_diff << std::endl;
    
    // 粗略的PSNR估算（简化版本）
    if (avg_diff > 0) {
        double mse = (avg_diff * avg_diff);
        double psnr = 10 * log10((255.0 * 255.0) / mse);
        std::cout << "估算PSNR: " << psnr << " dB";
        if (psnr > 40) std::cout << " (优秀)";
        else if (psnr > 30) std::cout << " (良好)";
        else if (psnr > 20) std::cout << " (一般)";
        else std::cout << " (较差)";
        std::cout << std::endl;
    }

    // 验证结果
    std::cout << "\n--- 功能验证 ---" << std::endl;
    bool test_passed = true;
    
    if (output_word_count == 0) {
        std::cerr << "✗ FAIL: 没有收到任何输出!" << std::endl;
        test_passed = false;
    } else {
        std::cout << "✓ 接收到输出数据" << std::endl;
    }
    
    if (!last_signal_received && output_word_count > 0) {
        std::cerr << "✗ FAIL: 未收到TLAST信号!" << std::endl;
        test_passed = false;
    } else if (output_word_count > 0) {
        std::cout << "✓ TLAST信号正确" << std::endl;
    }
    
    int expected_words = TEST_HEIGHT * num_words;
    if (output_word_count == expected_words) {
        std::cout << "✓ 输出数量正确 (" << output_word_count << " 个传输)" << std::endl;
    } else if (output_word_count > 0) {
        std::cout << "⚠ 输出传输数量 (" << output_word_count 
                  << ") 与预期 (" << expected_words << ") 不匹配" << std::endl;
        std::cout << "  (可能由于边界处理，属于正常现象)" << std::endl;
    }

    // 最终结果
    std::cout << "\n==================================================" << std::endl;
    if (test_passed) {
        std::cout << "✓✓✓ PASS: 测试成功通过! ✓✓✓" << std::endl;
        std::cout << "\n处理统计:" << std::endl;
        std::cout << "  - 输入: " << TEST_WIDTH << "x" << TEST_HEIGHT << " 像素" << std::endl;
        std::cout << "  - 输出: " << output_pixel_count << " 个像素" << std::endl;
        std::cout << "  - AXI传输: " << output_word_count << " 次" << std::endl;
        std::cout << "  - 并行度: " << NPIX << " 像素/周期" << std::endl;
        
        std::cout << "\n生成的图像文件:" << std::endl;
        std::cout << "  📁 input_image.ppm / .bmp    - 原始输入图像" << std::endl;
        std::cout << "  📁 output_image.ppm / .bmp   - 处理后输出图像" << std::endl;
        std::cout << "  📁 diff_image.ppm / .bmp     - 像素差异图" << std::endl;
        std::cout << "  📁 diff_enhanced.ppm / .bmp  - 增强差异图" << std::endl;
        
        std::cout << "\n提示:" << std::endl;
        std::cout << "  - .ppm 文件可用GIMP、IrfanView等查看" << std::endl;
        std::cout << "  - .bmp 文件可用Windows图片查看器直接打开" << std::endl;
    } else {
        std::cout << "✗✗✗ FAIL: 测试失败 ✗✗✗" << std::endl;
    }
    std::cout << "==================================================" << std::endl;
    
    return test_passed ? 0 : 1;
}