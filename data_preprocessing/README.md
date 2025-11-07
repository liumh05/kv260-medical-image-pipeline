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

## 技术规格

- 最大图像尺寸：3840×2160
- 像素格式：24位RGB (XF_8UC3)
- 像素并行度：1像素/时钟周期
- 数据位宽：24位

## 算法流程

图像处理按以下顺序执行：

1. **中值滤波**：对输入图像进行3x3中值滤波
2. **USM锐化**：对滤波结果进行锐化处理
3. **对比度与亮度增强**：对锐化结果应用对比度和亮度调整

## 配置参数

可通过修改Filter2D.cpp中的宏定义调整算法参数：

```cpp
// 锐化强度
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