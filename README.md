# 基于KV260的开源医学图像预处理、AI分割和三维重建项目

一个基于Xilinx KV260平台的端到端医学图像处理流水线系统，集成了数据预处理、AI分割和三维重建功能。

## 项目概述

本项目实现了一个完整的医学图像处理工作流程，从原始医学图像（DICOM/NIfTI格式）开始，经过预处理、AI分割、三维重建，最终生成可视化结果。项目采用软硬件协同设计，支持CPU和FPGA加速处理。

本项目参加了2025年全国嵌入式设计竞赛FPGA自主选题赛道。

### 主要功能

- **医学图像预处理**：中值滤波、USM锐化、对比度增强
- **AI图像分割**：基于UNet的深度学习分割模型
- **三维重建**：Marching Cubes算法实现表面重建
- **硬件加速**：FPGA加速的图像处理和AI推理

### 技术特点

- **端到端流水线**：从原始数据到3D可视化的完整处理
- **多模态支持**：支持CT、MRI等多种医学图像格式
- **高性能加速**：结合DPU和HLS实现硬件加速
- **模块化设计**：各功能模块独立，易于维护和扩展

## 项目结构

```
kv260-medical-image-pipeline/
├── ai_segmentation/           # AI分割模块
│   └── code2/                 # UNet分割算法实现
├── data_preprocessing/        # 数据预处理模块
│   ├── hls_src/              # HLS图像处理实现
│   └── hls_testbench/        # 仿真测试环境
├── marching_cubes_c/          # Marching Cubes C++实现
├── marching_cubes_hls/        # Marching Cubes HLS实现
├── jupyter_lab/              # Jupyter实验环境
│   ├── ai_cell/              # 细胞分割实验
│   ├── ai_liver/             # 肝脏分割实验
│   ├── preprocess/           # 数据预处理实验
│   └── reconstruction/       # 3D重建实验
└── README.md                 # 项目说明文档
```

## 核心模块

### 1. AI分割模块 (`ai_segmentation/`)

基于PyTorch的2D-UNet医学图像分割系统，支持细胞和肝脏分割任务。

**功能特性**：
- 高精度分割：Dice系数达98.02%
- 支持模型量化和硬件部署
- 完整的训练、评估、推理流程
- 支持Vitis AI部署和DPU加速

**快速开始**：
```bash
cd ai_segmentation/code2
pip install -r requirements.txt
./scripts/run_train.sh    # 训练模型
./scripts/run_quant.sh    # 量化模型
./scripts/run_inference.sh # 推理测试
```

### 2. 数据预处理模块 (`data_preprocessing/`)

基于HLS的实时图像预处理系统，针对FPGA优化设计。

**处理流程**：
1. **中值滤波**：3×3窗口去除脉冲噪声
2. **USM锐化**：增强图像细节（α=0.75）
3. **对比度增强**：调整对比度和亮度（α=2.0, β=15）

**技术规格**：
- 最大图像尺寸：3840×2160
- 像素格式：24位RGB
- 流水线处理：1像素/时钟周期

### 3. 三维重建模块 (`marching_cubes_*`)

提供C++和HLS两种实现，从体数据提取等值面网格。

**C++版本**：
- 纯C++17实现，无第三方依赖
- 支持多种数据类型（float32、int16、int32、uint8）
- 输入：NPY格式体数据
- 输出：VTK格式网格文件

**HLS版本**：
- FPGA硬件加速实现
- 支持流式处理
- 最大支持128³体数据
- 高性能并行处理

### 4. Jupyter实验环境 (`jupyter_lab/`)

提供完整的交互式开发环境，包含各个模块的使用示例。

**实验内容**：
- 数据预处理流程演示
- AI分割模型推理验证
- 三维重建可视化
- 硬件加速性能测试

## 完整工作流程

### 1. 数据准备
```bash
# DICOM/NIfTI到NPY格式转换
python jupyter_lab/preprocess/transform_nii_npy.py --input medical_data.nii --output volume.npy
```

### 2. 图像预处理
```cpp
// HLS C++实现
#include "Filter2D.h"
Filter2D_accel(src_stream, dst_stream, height, width);
```

### 3. AI分割推理
```python
# Python PyTorch实现
from networks.UNet import UNet
model = UNet(n_channels=1, n_classes=2)
prediction = model(input_tensor)
```

### 4. 三维重建
```cpp
// Marching Cubes实现
marching_cubes(volume, width, height, depth, isovalue, vertices, triangles);
```

### 5. 结果可视化
```python
# PyVista可视化
import pyvista as pv
mesh = pv.read('output.vtk')
mesh.plot()
```

## 环境要求

### 软件环境
- Python 3.8+
- PyTorch 1.7+
- OpenCV 4.0+
- CMake 3.15+
- C++17编译器

### 硬件环境
- Xilinx KV260开发板
- DDR内存 ≥ 4GB
- 可选：支持CUDA的GPU（用于训练加速）

## 安装和配置

### 1. 克隆项目
```bash
git clone <repository_url>
cd kv260-medical-image-pipeline
```

### 2. 安装Python依赖
```bash
# 安装基础依赖
pip install torch torchvision
pip install opencv-python
pip install numpy scipy pandas
pip install matplotlib plotly pyvista
pip install pynq dpu-pynq

# 安装AI分割模块依赖
cd ai_segmentation/code2
pip install -r requirements.txt
```

### 3. 构建C++模块
```bash
# 构建Marching Cubes C++版本
cd marching_cubes_c
mkdir build && cd build
cmake ..
make -j

# HLS综合（需要Vitis HLS）
cd ../marching_cubes_hls/hls_src
vitis_hls -p marching_cubes_hls
```

### 4. 配置FPGA
```bash
# 加载KV260 bitstream
cd jupyter_lab/ai_liver
python liver_segmentation_dpu.ipynb  # 参考配置流程
```

## 快速开始

### 1. 运行完整流程示例
```bash
# 启动Jupyter环境
cd jupyter_lab
jupyter lab

# 运行预处理实验
jupyter_lab/preprocess/preprocess.ipynb

# 运行AI分割实验
jupyter_lab/ai_liver/liver_segmentation_dpu.ipynb

# 运行3D重建实验
jupyter_lab/reconstruction/3D_Reconstruction.ipynb
```

### 2. 命令行快速测试
```bash
# C++ Marching Cubes测试
cd marching_cubes_c
./build/marching_cubes --input case_00000_x.npy --iso 0.5 --vtk output.vtk

# HLS仿真测试
cd data_preprocessing/hls_testbench
g++ -I../hls_src Filter2D_tb.cpp -o test
./test
```

## 许可证

本项目基于MIT许可证开源，详见LICENSE文件。

## 致谢

- Xilinx提供的PYNQ和Vitis AI工具链
- PyTorch深度学习框架
- OpenCV计算机视觉库
- Marching Cubes算法原始作者

## 联系方式

如有问题或建议，请通过以下方式联系：
- 邮件联系：1964722203@qq.com
