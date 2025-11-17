# PyTorch UNet CHAOS-CT 医学图像分割模块

## 项目简介

这是一个基于PyTorch的2D-UNet轻量级网络实现。模块提供从训练到量化的完整流程，支持Vitis AI部署。

## 目录结构

```
code2/
├── train/                  # 训练相关代码
│   ├── train.py           # 训练主程序
│   └── train_options.py   # 训练参数配置
├── test/                   # 测试和推理代码
│   ├── test.py            # 测试和量化主程序
│   ├── inference.py       # 独立推理程序
│   └── predict.py         # 预测工具
├── networks/               # 网络架构
│   ├── UNet.py            # UNet主网络定义
│   ├── UNet_parts.py      # UNet构建模块
│   ├── build_model.py     # 模型构建器
│   ├── evaluate.py        # 评估函数
│   └── utils.py           # 网络工具函数
├── dataset/                # 数据处理
│   └── datasets.py        # 数据集加载器
├── utils/                  # 工具函数
│   ├── criterion.py       # 损失函数
│   ├── eval_dice_miou.py  # 评估指标计算
│   ├── flops_compute.py   # FLOPs计算
│   ├── prepare_data.py    # 数据预处理
│   ├── train_options.py   # 训练参数
│   └── utils.py           # 通用工具函数
├── scripts/                # 运行脚本
│   ├── run_train.sh       # 训练启动脚本
│   ├── run_eval.sh        # 评估启动脚本
│   ├── run_quant.sh       # 量化启动脚本
│   └── run_inference.sh   # 推理启动脚本
├── models/                 # 模型文件
│   └── ChaosCT_0.9770885167.pth  # 预训练模型
├── config/                 # 配置文件
│   ├── quant_info.json    # 量化配置信息
│   └── bias_corr.pth      # 量化偏置校正
├── README.md               # 项目说明文档
└── requirements.txt        # Python依赖列表
```

## 快速开始

### 环境要求
- Python 3.6+
- PyTorch 1.7.1+
- CUDA (推荐)
- Vitis AI

### 安装依赖
```bash
pip install torch torchvision
pip install opencv-python numpy tqdm
pip install SimpleITK pandas pillow matplotlib
pip install vai_q_pytorch  # 可选，用于量化
```

### 数据准备
1. 下载CHAOS-CT数据集 (https://chaos.grand-challenge.org/Download/)
2. 使用数据预处理工具转换DICOM到PNG格式:
   ```bash
   python utils/prepare_data.py
   ```
3. 组织数据目录结构:
   ```
   data/
   └── Chaos/
       ├── Train_Sets/CT/          # 训练集: [1,2,5,6,8,10,14,16,18,19,21,22,23,24,25,26,27,28,29,30]
       │   ├── DICOM_anon/         # 原始DICOM文件
       │   ├── Ground/             # 真实标签
       │   └── image/              # 转换后的PNG图像
       └── Test_Sets/CT/           # 测试集: [3,4,7,9,11,12,13,15,17,20,31,32,33,34,35,36,37,38,39,40]
           ├── DICOM_anon/
           ├── Ground/
           └── image/
   ```

### 训练模型
```bash
cd scripts/
bash run_train.sh
```

### 模型评估
```bash
cd scripts/
bash run_eval.sh
```

### 模型量化
```bash
cd scripts/
bash run_quant.sh
```

### 模型推理
```bash
cd scripts/
bash run_inference.sh
```

## 核心功能

### 训练 (train/)
- **train.py**: 主训练程序，支持学习率调度、数据增强
- **train_options.py**: 训练参数配置和命令行解析

### 测试和量化 (test/)
- **test.py**: 多模式测试程序，支持float/calib/test/dump_xmodel模式
- **inference.py**: 独立推理工具，支持批量处理
- **predict.py**: 轻量级预测工具

### 网络架构 (networks/)
- **UNet.py**: 完整的UNet网络定义
- **UNet_parts.py**: UNet的基础构建模块
- **build_model.py**: 模型构建和初始化

### 数据处理 (dataset/, utils/)
- **datasets.py**: 训练和验证数据集加载器
- **prepare_data.py**: 数据预处理工具
- **criterion.py**: 损失函数定义

## 性能指标

- **输入尺寸**: 512×512 CT图像
- **模型复杂度**: 23.3G FLOPs
- **浮点精度**: 98.02% Dice (验证集), 97.58% Dice (测试集)
- **INT8量化**: 98.11% Dice (验证集), 97.47% Dice (测试集)

## 文件说明

### 核心文件
- `train/train.py`: 训练主程序
- `test/test.py`: 测试和量化程序
- `networks/UNet.py`: UNet网络定义
- `dataset/datasets.py`: 数据加载器
- `inference.py`: 推理程序

### 配置文件
- `config/quant_info.json`: 量化配置参数
- `config/bias_corr.pth`: 量化偏置校正参数

### 模型文件
- `models/ChaosCT_0.9770885167.pth`: 预训练模型权重

### 脚本文件
- `scripts/run_*.sh`: 各种运行脚本，设置正确的环境变量和参数

## 使用说明

1. **训练**: 使用`scripts/run_train.sh`启动训练
2. **评估**: 使用`scripts/run_eval.sh`评估模型性能
3. **量化**: 使用`scripts/run_quant.sh`进行模型量化
4. **推理**: 使用`scripts/run_inference.sh`进行模型推理

## 注意事项

- 确保数据目录结构正确
- 训练前检查CUDA是否可用
- 量化需要Vitis AI环境
- 推理时确保模型文件存在

## 技术规格

- **网络架构**: 2D-UNet轻量级版本
- **输入尺寸**: 512×512 CT图像
- **模型复杂度**: 23.3G FLOPs
- **预处理**: BGR格式, 均值减法[104, 117, 123]
- **数据增强**: 随机镜像、缩放

## 数据预处理说明

- **图像格式**: DICOM → PNG转换
- **窗口设置**: CT窗宽窗位优化
- **尺寸标准化**: 统一调整至512×512
- **颜色空间**: BGR格式 (OpenCV标准)

## Vitis AI集成

- **量化流程**: 校准 → 测试 → XModel导出
- **精度保持**: 偏置校正优化
- **部署支持**: 生成DPU兼容的XModel文件

## 注意事项

- 确保数据目录结构正确
- 训练前检查CUDA是否可用
- 量化需要Vitis AI环境 (`conda activate vitis-ai-pytorch`)
- 推理时确保模型文件存在