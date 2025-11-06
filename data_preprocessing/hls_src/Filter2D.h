#ifndef _FILTER_2D_H_
#define _FILTER_2D_H_

#include "hls_stream.h"
#include "ap_axi_sdata.h"
#include "common/xf_common.hpp"

// 图像最大尺寸
#define WIDTH 3840
#define HEIGHT 2160

// 像素并行度 (一次处理一个像素)
#define NPIX XF_NPPC1

// 图像类型定义
#define IN_TYPE      XF_8UC3
#define OUT_TYPE     XF_8UC3

// AXI Stream 数据宽度
#define DATA_WIDTH (XF_PIXELWIDTH(IN_TYPE, NPIX))

// AXI Stream 类型定义
typedef ap_axiu<DATA_WIDTH, 1, 1, 1> interface_t;
typedef hls::stream<interface_t> stream_t;

// 顶层函数声明
void Filter2D_accel(stream_t& src, stream_t& dst, int height, int width);

#endif // _FILTER_2D_H_