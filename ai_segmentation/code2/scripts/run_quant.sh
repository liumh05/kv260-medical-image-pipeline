#!/bin/bash
# UNet量化脚本 - 执行完整的量化流程

# 设置Python路径和量化环境变量
export PYTHONPATH=${PWD}/..:${PYTHONPATH}
export W_QUANT=1

# 步骤1: 校准
echo "=== 步骤1: 模型校准 ==="
CUDA_VISIBLE_DEVICES='0' python ../test/test.py  \
        --data_root '../../data/Chaos/Train_Sets/CT' \
        --input_size 512,512 \
        --weight '../models/ChaosCT_0.9770885167.pth' \
        --quant_mode calib

# 步骤2: 测试量化模型
echo "=== 步骤2: 测试量化模型 ==="
CUDA_VISIBLE_DEVICES='0' python ../test/test.py  \
        --data_root '../../data/Chaos/Train_Sets/CT' \
        --input_size 512,512 \
        --weight '../models/ChaosCT_0.9770885167.pth' \
        --quant_mode test

# 步骤3: 导出XModel模型
echo "=== 步骤3: 导出XModel ==="
CUDA_VISIBLE_DEVICES='0' python ../test/test.py  \
        --dump_xmodel \
        --data_root '../../data/Chaos/Train_Sets/CT' \
        --input_size 512,512 \
        --weight '../models/ChaosCT_0.9770885167.pth' \
        --quant_mode test

echo "=== 量化流程完成 ==="
