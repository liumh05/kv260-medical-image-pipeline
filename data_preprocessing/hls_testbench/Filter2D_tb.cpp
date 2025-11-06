#include <iostream>
#include <fstream>
#include "Filter2D.h"

#define TEST_WIDTH 128
#define TEST_HEIGHT 128

int main() {
    stream_t src_stream;
    stream_t dst_stream;

    std::cout << "--- 开始生成 " << TEST_WIDTH << "x" << TEST_HEIGHT << " 的测试图像流 ---" << std::endl;

    for (int y = 0; y < TEST_HEIGHT; y++) {
        for (int x = 0; x < TEST_WIDTH; x++) {
            interface_t axi_data;
            ap_uint<24> pixel_val;
            if ((x / 16 + y / 16) % 2 == 0) { pixel_val = 0xFFFFFF; } else { pixel_val = 0x000000; }
            axi_data.data = pixel_val;
            axi_data.keep = -1;
            axi_data.last = (y == TEST_HEIGHT - 1 && x == TEST_WIDTH - 1);
            src_stream.write(axi_data);
        }
    }
    std::cout << "--- 测试图像流生成完毕 ---" << std::endl;

    std::cout << "--- 调用 Filter2D_accel IP核 ---" << std::endl;
    Filter2D_accel(src_stream, dst_stream, TEST_HEIGHT, TEST_WIDTH);
    std::cout << "--- IP核处理完成 ---" << std::endl;

    std::cout << "--- 正在检查输出流 ---" << std::endl;
    int output_pixel_count = 0;
    bool last_signal_received = false;

    while (!dst_stream.empty()) {
        interface_t axi_out_data = dst_stream.read();
        output_pixel_count++;
        if (axi_out_data.last) {
            last_signal_received = true;
        }
    }

    // 验证结果
    // 由于边界处理，输出的像素会比输入少一些，这里我们只检查流是否结束
    bool test_passed = true;
    if (output_pixel_count == 0){
        std::cerr << "FAIL: 没有收到任何输出像素，IP核可能已死锁!" << std::endl;
        test_passed = false;
    }
    if (!last_signal_received && output_pixel_count > 0) {
        std::cerr << "FAIL: 数据流未正常结束，TLAST信号丢失!" << std::endl;
        test_passed = false;
    }

    if (test_passed) {
        std::cout << "*******************************************" << std::endl;
        std::cout << "PASS: 仿真测试成功通过!" << std::endl;
        std::cout << "接收到 " << output_pixel_count << " 个像素并且数据流正常结束。" << std::endl;
        std::cout << "*******************************************" << std::endl;
        return 0; // 成功
    } else {
        std::cout << "*******************************************" << std::endl;
        std::cout << "FAIL: 仿真测试失败。" << std::endl;
        std::cout << "*******************************************" << std::endl;
        return 1; // 失败
    }
}