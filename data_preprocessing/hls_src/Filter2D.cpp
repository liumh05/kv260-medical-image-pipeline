#include "Filter2D.h"
#include "hls_math.h"

// --- 锐化强度控制 ---
#define SHARPEN_STRENGTH_NUMERATOR 3
#define SHARPEN_STRENGTH_DENOMINATOR 4

// --- 对比度和亮度控制 ---
#define CONTRAST_ALPHA_NUMERATOR 20
#define CONTRAST_ALPHA_DENOMINATOR 10
#define BRIGHTNESS_BETA 15

// 辅助函数：比较和交换
template <typename T>
void compare_and_swap(T& a, T& b) {
#pragma HLS INLINE
    if (a > b) {
        T temp = a; a = b; b = temp;
    }
}

// 中值滤波排序网络
void median_filter_network(ap_uint<8> win_ch[9], ap_uint<8>& median) {
#pragma HLS INLINE
    compare_and_swap(win_ch[0], win_ch[1]); 
    compare_and_swap(win_ch[3], win_ch[4]); 
    compare_and_swap(win_ch[6], win_ch[7]);
    compare_and_swap(win_ch[1], win_ch[2]); 
    compare_and_swap(win_ch[4], win_ch[5]); 
    compare_and_swap(win_ch[7], win_ch[8]);
    compare_and_swap(win_ch[0], win_ch[1]); 
    compare_and_swap(win_ch[3], win_ch[4]); 
    compare_and_swap(win_ch[6], win_ch[7]);
    compare_and_swap(win_ch[0], win_ch[3]); 
    compare_and_swap(win_ch[1], win_ch[4]); 
    compare_and_swap(win_ch[2], win_ch[5]);
    compare_and_swap(win_ch[3], win_ch[6]); 
    compare_and_swap(win_ch[4], win_ch[7]); 
    compare_and_swap(win_ch[5], win_ch[8]);
    compare_and_swap(win_ch[1], win_ch[4]); 
    compare_and_swap(win_ch[2], win_ch[5]);
    compare_and_swap(win_ch[0], win_ch[3]); 
    compare_and_swap(win_ch[1], win_ch[4]); 
    compare_and_swap(win_ch[3], win_ch[6]);
    compare_and_swap(win_ch[4], win_ch[7]);
    compare_and_swap(win_ch[2], win_ch[4]); 
    compare_and_swap(win_ch[5], win_ch[7]);
    compare_and_swap(win_ch[1], win_ch[3]);
    compare_and_swap(win_ch[2], win_ch[4]);
    median = win_ch[4];
}

// 单个通道的滤波处理
template<int W>
void process_channel(ap_uint<W> window_3x3[3][3], int c, ap_uint<8>& result) {
#pragma HLS INLINE off
    
    // 提取3x3窗口的指定通道
    ap_uint<8> win_ch[9];
    int idx = 0;
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            win_ch[idx++] = window_3x3[i][j].range(c*8+7, c*8);
        }
    }
    
    ap_uint<8> blurred_ch;
    median_filter_network(win_ch, blurred_ch);
    
    ap_uint<8> original_ch = window_3x3[1][1].range(c*8+7, c*8);
    ap_int<9> detail_ch = original_ch - blurred_ch;
    ap_int<9> scaled_detail = (detail_ch * SHARPEN_STRENGTH_NUMERATOR) / SHARPEN_STRENGTH_DENOMINATOR;
    ap_int<10> sharpened_val = original_ch + scaled_detail;
    
    // 锐化饱和处理
    ap_uint<8> sharpened_ch;
    if (sharpened_val > 255) sharpened_ch = 255;
    else if (sharpened_val < 0) sharpened_ch = 0;
    else sharpened_ch = sharpened_val;
    
    // 对比度与亮度增强
    ap_int<16> contrast_val = (sharpened_ch * CONTRAST_ALPHA_NUMERATOR) / CONTRAST_ALPHA_DENOMINATOR + BRIGHTNESS_BETA;
    
    // 饱和处理
    if (contrast_val > 255) result = 255;
    else if (contrast_val < 0) result = 0;
    else result = contrast_val;
}

// 顶层函数
void Filter2D_accel(stream_t& src, stream_t& dst, int height, int width) {
#pragma HLS INTERFACE axis register both port=src
#pragma HLS INTERFACE axis register both port=dst
#pragma HLS INTERFACE s_axilite port=height
#pragma HLS INTERFACE s_axilite port=width
#pragma HLS INTERFACE s_axilite port=return

    // 计算处理的word数量
    const int cols = width / NPIX;
    
    // 行缓存：存储两行数据
    ap_uint<PIXEL_WIDTH> line_buffer[2][WIDTH + 2*NPIX];
#pragma HLS ARRAY_PARTITION variable=line_buffer complete dim=1
#pragma HLS BIND_STORAGE variable=line_buffer type=ram_t2p impl=bram

    // 滑动窗口
    ap_uint<PIXEL_WIDTH> window[3][2+NPIX];
#pragma HLS ARRAY_PARTITION variable=window complete dim=0

    // 初始化line_buffer
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < WIDTH + 2*NPIX; j++) {
#pragma HLS PIPELINE II=1
            line_buffer[i][j] = 0;
        }
    }
    
    // 初始化window
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 2+NPIX; j++) {
            window[i][j] = 0;
        }
    }
    
    // 主处理循环
    for (int y = 0; y < height + 1; y++) {
        for (int x = 0; x < cols + 1; x++) {
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_FLATTEN

            // 1. 读取输入数据
            ap_uint<PIXEL_WIDTH> current_pixels[NPIX];
#pragma HLS ARRAY_PARTITION variable=current_pixels complete
            
            if (y < height && x < cols) {
                interface_t axi_in = src.read();
                for(int p = 0; p < NPIX; p++) {
#pragma HLS UNROLL
                    current_pixels[p] = axi_in.data.range((p+1)*PIXEL_WIDTH-1, p*PIXEL_WIDTH);
                }
            } else {
                for(int p = 0; p < NPIX; p++) {
#pragma HLS UNROLL
                    current_pixels[p] = 0;
                }
            }
            
            // 2. 更新行缓存和窗口
            ap_uint<PIXEL_WIDTH> top_pixels[NPIX+2];
            ap_uint<PIXEL_WIDTH> middle_pixels[NPIX+2];
#pragma HLS ARRAY_PARTITION variable=top_pixels complete
#pragma HLS ARRAY_PARTITION variable=middle_pixels complete
            
            int buffer_idx = x * NPIX;
            for(int p = 0; p < NPIX+2; p++) {
#pragma HLS UNROLL
                top_pixels[p] = line_buffer[0][buffer_idx + p];
                middle_pixels[p] = line_buffer[1][buffer_idx + p];
            }
            
            for(int p = 0; p < NPIX; p++) {
#pragma HLS UNROLL
                int idx = buffer_idx + p + 1;
                line_buffer[0][idx] = middle_pixels[p+1];
                line_buffer[1][idx] = current_pixels[p];
            }
            
            // 滑动窗口
            for(int i = 0; i < 3; i++) {
#pragma HLS UNROLL
                for(int j = 0; j < 2; j++) {
#pragma HLS UNROLL
                    window[i][j] = window[i][j+NPIX];
                }
            }
            
            for(int p = 0; p < NPIX; p++) {
#pragma HLS UNROLL
                window[0][2+p] = top_pixels[p+1];
                window[1][2+p] = middle_pixels[p+1];
                window[2][2+p] = current_pixels[p];
            }
            
            // 3. 处理像素
            if (y > 0 && x > 0) {
                ap_uint<PIXEL_WIDTH> output_pixels[NPIX];
#pragma HLS ARRAY_PARTITION variable=output_pixels complete
                
                for(int p = 0; p < NPIX; p++) {
//#pragma HLS UNROLL  // 暂时不展开此循环
                    
                    int actual_x = (x-1) * NPIX + p;
                    int actual_y = y - 1;
                    
                    bool is_border = (actual_y < 1 || actual_x < 1 || 
                                     actual_y >= height-1 || actual_x >= width-1);
                    
                    if (is_border) {
                        output_pixels[p] = window[1][1+p];
                    } else {
                        // 提取3x3窗口
                        ap_uint<PIXEL_WIDTH> window_3x3[3][3];
                        
                        for(int i = 0; i < 3; i++) {
                            for(int j = 0; j < 3; j++) {
                                window_3x3[i][j] = window[i][p+j];
                            }
                        }
                        
                        // 处理三个通道
                        ap_uint<PIXEL_WIDTH> filtered_pixel = 0;
                        for(int c = 0; c < 3; c++) {
                            ap_uint<8> result_ch;
                            process_channel<PIXEL_WIDTH>(window_3x3, c, result_ch);
                            filtered_pixel.range(c*8+7, c*8) = result_ch;
                        }
                        
                        output_pixels[p] = filtered_pixel;
                    }
                }
                
                // 4. 写输出
                interface_t axi_out;
                for(int p = 0; p < NPIX; p++) {
#pragma HLS UNROLL
                    axi_out.data.range((p+1)*PIXEL_WIDTH-1, p*PIXEL_WIDTH) = output_pixels[p];
                }
                axi_out.keep = -1;
                axi_out.last = ((y-1) == (height-1) && x == cols);
                dst.write(axi_out);
            }
        }
    }
}