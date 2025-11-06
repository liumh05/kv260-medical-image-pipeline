#!/bin/bash
export PYTHONPATH=${PWD}:${PYTHONPATH}

# 设置CUDA设备
export CUDA_VISIBLE_DEVICES='0'

# 运行推理脚本
python inference.py \
    --model_path '../float/ChaosCT_0.9770885167.pth' \
    --test_dir '../data/Chaos/Test_Sets/CT' \
    --output_dir './results' \
    --device 'cuda' \
    --save_overlay

echo "推理完成！结果保存在 ./results 目录中"