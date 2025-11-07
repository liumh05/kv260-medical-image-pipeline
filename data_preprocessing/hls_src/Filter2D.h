#ifndef _FILTER_2D_H_
#define _FILTER_2D_H_

#include "hls_stream.h"
#include "ap_axi_sdata.h"
#include "common/xf_common.hpp"

// 图像最大尺寸
#define WIDTH 3840
#define HEIGHT 2160

// ============================================
// 像素并行度配置
// ============================================
#define NPIX XF_NPPC4  // 一次处理4个像素

// 图像类型定义
#define IN_TYPE      XF_8UC3
#define OUT_TYPE     XF_8UC3

// 单个像素的数据宽度（固定24位 = 8位×3通道）
#define PIXEL_WIDTH 24

// ============================================
// AXI Stream 数据宽度（直接写死，避免宏展开问题）
// 如果修改NPIX，必须同时修改这个值！
// NPIX=1 → 24, NPIX=2 → 48, NPIX=4 → 96, NPIX=8 → 192
// ============================================
#define DATA_WIDTH 96  // 当前配置：4个像素 × 24位/像素

// AXI Stream 类型定义
typedef ap_axiu<DATA_WIDTH, 1, 1, 1> interface_t;
typedef hls::stream<interface_t> stream_t;

// 顶层函数声明
void Filter2D_accel(stream_t& src, stream_t& dst, int height, int width);

#endif // _FILTER_2D_H_