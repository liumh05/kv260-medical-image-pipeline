#include "Filter2D.h"
#include "hls_math.h"

// --- 锐化强度控制 ---
// alpha = 0.75
#define SHARPEN_STRENGTH_NUMERATOR 3
#define SHARPEN_STRENGTH_DENOMINATOR 4

// --- 新增：对比度和亮度控制 ---
// 1. 对比度 (alpha > 1 增强, < 1 减弱)
//    这里设置为 1.1
#define CONTRAST_ALPHA_NUMERATOR 20
#define CONTRAST_ALPHA_DENOMINATOR 10
// 2. 亮度 (beta > 0 增亮, < 0 减暗)
//    这里设置为 10，轻微提亮
#define BRIGHTNESS_BETA 15


// 辅助函数：比较和交换
template <typename T>
void compare_and_swap(T& a, T& b) {
#pragma HLS INLINE
    if (a > b) {
        T temp = a; a = b; b = temp;
    }
}

// 顶层函数
void Filter2D_accel(stream_t& src, stream_t& dst, int height, int width) {
#pragma HLS INTERFACE axis register both port=src
#pragma HLS INTERFACE axis register both port=dst
#pragma HLS INTERFACE s_axilite port=height
#pragma HLS INTERFACE s_axilite port=width
#pragma HLS INTERFACE s_axilite port=return

    // ... (行缓存和窗口声明部分，与之前完全相同) ...
    ap_uint<DATA_WIDTH> line_buffer[2][WIDTH];
#pragma HLS ARRAY_PARTITION variable=line_buffer complete dim=1

    ap_uint<DATA_WIDTH> window[3][3];
#pragma HLS ARRAY_PARTITION variable=window complete dim=0

    // 遍历所有像素
    for (int y = 0; y < height + 1; y++) {
        for (int x = 0; x < width + 1; x++) {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_FLATTEN

            // ... (1. 读数据 和 2. 更新窗口部分，与之前完全相同) ...
            interface_t axi_in;
            ap_uint<DATA_WIDTH> current_pixel;
            if (y < height && x < width) {
                axi_in = src.read();
                current_pixel = axi_in.data;
            }

            ap_uint<DATA_WIDTH> top_pixel = (x < width) ? line_buffer[0][x] : (ap_uint<DATA_WIDTH>)0;
            ap_uint<DATA_WIDTH> middle_pixel = (x < width) ? line_buffer[1][x] : (ap_uint<DATA_WIDTH>)0;
            if(x < width) {
                line_buffer[0][x] = middle_pixel;
                line_buffer[1][x] = current_pixel;
            }

            for(int i = 0; i < 3; i++) for(int j = 0; j < 2; j++) window[i][j] = window[i][j+1];
            window[0][2] = top_pixel;
            window[1][2] = middle_pixel;
            window[2][2] = current_pixel;


            // 3. 计算与输出
            ap_uint<DATA_WIDTH> output_pixel;
            bool is_valid_pixel = (y > 0 && x > 0);

            if (is_valid_pixel) {
                if (y < 2 || x < 2 || y > height -1 || x > width -1) {
                    output_pixel = window[1][1];
                } else {
                    ap_uint<DATA_WIDTH> final_pixel;
                    
                    for (int c = 0; c < 3; c++) {
#pragma HLS UNROLL
                        // --- 步骤 A: 锐化处理 (Unsharp Masking) ---
                        ap_uint<8> win_ch[9];
                        for(int i = 0; i < 3; i++) for(int j = 0; j < 3; j++) win_ch[i*3+j] = window[i][j].range(c*8+7, c*8);
                        
                        compare_and_swap(win_ch[0], win_ch[1]); compare_and_swap(win_ch[3], win_ch[4]); compare_and_swap(win_ch[6], win_ch[7]);
                        compare_and_swap(win_ch[1], win_ch[2]); compare_and_swap(win_ch[4], win_ch[5]); compare_and_swap(win_ch[7], win_ch[8]);
                        compare_and_swap(win_ch[0], win_ch[1]); compare_and_swap(win_ch[3], win_ch[4]); compare_and_swap(win_ch[6], win_ch[7]);
                        compare_and_swap(win_ch[0], win_ch[3]); compare_and_swap(win_ch[1], win_ch[4]); compare_and_swap(win_ch[2], win_ch[5]);
                        compare_and_swap(win_ch[3], win_ch[6]); compare_and_swap(win_ch[4], win_ch[7]); compare_and_swap(win_ch[5], win_ch[8]);
                        compare_and_swap(win_ch[1], win_ch[4]); compare_and_swap(win_ch[2], win_ch[5]);
                        compare_and_swap(win_ch[0], win_ch[3]); compare_and_swap(win_ch[1], win_ch[4]); compare_and_swap(win_ch[3], win_ch[6]);
                        compare_and_swap(win_ch[4], win_ch[7]);
                        compare_and_swap(win_ch[2], win_ch[4]); compare_and_swap(win_ch[5], win_ch[7]);
                        compare_and_swap(win_ch[1], win_ch[3]);
                        compare_and_swap(win_ch[2], win_ch[4]);
                        ap_uint<8> blurred_ch = win_ch[4];

                        ap_uint<8> original_ch = window[1][1].range(c*8+7, c*8);
                        ap_int<9> detail_ch = original_ch - blurred_ch;
                        ap_int<9> scaled_detail = (detail_ch * SHARPEN_STRENGTH_NUMERATOR) / SHARPEN_STRENGTH_DENOMINATOR;
                        ap_int<10> sharpened_val = original_ch + scaled_detail;
                        
                        // 锐化结果饱和处理
                        ap_uint<8> sharpened_ch;
                        if (sharpened_val > 255) sharpened_ch = 255;
                        else if (sharpened_val < 0) sharpened_ch = 0;
                        else sharpened_ch = sharpened_val;

                        // ######################################################
                        // ### 新增步骤 B: 对比度与亮度增强 ###
                        // ######################################################
                        // 应用公式: g(x) = alpha * f(x) + beta
                        // f(x)就是我们上一步得到的 sharpened_ch
                        ap_int<16> contrast_val = (sharpened_ch * CONTRAST_ALPHA_NUMERATOR) / CONTRAST_ALPHA_DENOMINATOR + BRIGHTNESS_BETA;
                        
                        // 对比度增强结果进行饱和处理
                        if (contrast_val > 255) {
                            final_pixel.range(c*8+7, c*8) = 255;
                        } else if (contrast_val < 0) {
                            final_pixel.range(c*8+7, c*8) = 0;
                        } else {
                            final_pixel.range(c*8+7, c*8) = contrast_val;
                        }
                    }
                    output_pixel = final_pixel;
                }
                
                // ... (4. 写数据部分，与之前完全相同) ...
                interface_t axi_out;
                axi_out.data = output_pixel;
                axi_out.keep = -1;
                axi_out.last = ((y - 1) == (height - 1) && (x - 1) == (width - 1));
                dst.write(axi_out);
            }
        }
    }
}