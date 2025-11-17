# 医学图像预处理模块

本项目实现了一个基于HLS的医学图像预处理系统，支持中值滤波、USM锐化、对比度增强和亮度调整功能。

## 项目结构

```
data_preprocessing/
├── hls_src/
│   ├── Filter2D.h             # 头文件定义
│   └── Filter2D.cpp           # 核心算法实现
├── hls_testbench/
│   └── Filter2D_tb.cpp        # C++仿真测试代码
└── vivado_design/
    └── Filter.tcl              # Block Design Tcl脚本
```

## 功能说明

### 1. 中值滤波
使用3x3窗口的中值滤波算法，通过快速排序查找中值，用于去除图像中的脉冲噪声。

实现方式：
- 使用行缓存存储图像数据
- 3x3滑动窗口处理
- 基于比较交换的快速中值查找

### 2. USM锐化
Unsharp Masking锐化算法，通过原图减去模糊图得到细节，再将细节叠加回原图。

实现公式：
```
detail = original - blurred
sharpened = original + α × detail
```

当前参数：α = 0.75

### 3. 对比度增强
基于线性变换的对比度调整。

实现公式：
```
g(x) = α × f(x) + β
```

当前参数：
- 对比度系数 α = 2.0
- 亮度偏移 β = 15

### 4. 亮度调整
通过对比度增强公式中的β参数实现亮度调整功能。

## HLS优化方法

### 1. 像素并行处理优化
- **NPIX配置**: 一次并行处理4个像素（`#define NPIX XF_NPPC4`）
- **数据宽度优化**: 96位AXI总线宽度（4像素×24位/像素）
- **吞吐量提升**: 理论上达到4倍像素处理性能

### 2. 存储器优化策略
- **行缓存设计**: 双行缓存减少外部存储访问
- **BRAM优化**: 使用`BIND_STORAGE`指令将行缓存映射到BRAM
- **数组分区**: 完全分区关键数组实现并行访问
  ```cpp
  #pragma HLS ARRAY_PARTITION variable=line_buffer complete dim=1
  #pragma HLS ARRAY_PARTITION variable=window complete dim=0
  ```

### 3. 流水线优化
- **II=1流水线**: 主处理循环实现每个时钟周期处理一个像素
- **循环扁平化**: 使用`LOOP_FLATTEN`减少循环开销
- **关键路径优化**: 内联小函数减少调用开销

### 4. 中值滤波算法优化
- **排序网络**: 使用9输入排序网络替代传统排序算法
- **比较交换优化**: 硬件友好的比较交换实现
- **固定延迟**: 确定31个比较周期的固定延迟

### 5. 边界处理优化
- **边界复制**: 自动检测并复制边界像素
- **条件分支优化**: 使用硬件友好的条件判断
- **零填充**: 边界外区域自动填充零值

### 6. 资源利用优化
- **UNROLL指令**: 关键循环完全展开
- **位宽精确控制**: 使用`ap_uint`精确控制数据位宽
- **饱和运算**: 防止溢出的饱和处理逻辑

## 性能指标

### 理论性能
- **像素吞吐量**: 4像素/时钟周期
- **处理延迟**: 每像素3时钟周期（考虑3x3窗口）
- **最大帧率**: 3840×2160@60fps支持

### 资源占用
- **BRAM利用率**: 双行缓存约2×(3840+8)×24位
- **DSP利用率**: 无DSP依赖，纯逻辑实现
- **LUT利用率**: 主要用于比较交换网络和饱和运算

## 配置参数

可通过修改Filter2D.cpp中的宏定义调整算法参数：

```cpp
// 像素并行度（影响性能和资源占用）
#define NPIX XF_NPPC4  // 可选：XF_NPPC1, XF_NPPC2, XF_NPPC4, XF_NPPC8

// 锐化强度（USM参数）
#define SHARPEN_STRENGTH_NUMERATOR 3
#define SHARPEN_STRENGTH_DENOMINATOR 4

// 对比度和亮度
#define CONTRAST_ALPHA_NUMERATOR 20
#define CONTRAST_ALPHA_DENOMINATOR 10
#define BRIGHTNESS_BETA 15
```

## 接口说明

### 顶层接口
```cpp
void Filter2D_accel(stream_t& src, stream_t& dst, int height, int width);
```

### 参数说明
- `src`: 输入AXI Stream
- `dst`: 输出AXI Stream
- `height`: 图像高度
- `width`: 图像宽度

## 使用方法

### 1. HLS综合
在Vitis HLS中打开hls_src目录，运行C仿真和C/RTL综合。

### 2. 仿真测试
编译并运行hls_testbench中的测试代码验证功能正确性。

### 3. Vivado集成
使用vivado_design中的Tcl脚本创建Block Design，集成到系统中。

## 注意事项

- 边界像素采用边界复制策略处理
- 所有中间计算使用饱和运算防止溢出
- 处理顺序固定：中值滤波 → USM锐化 → 对比度亮度增强