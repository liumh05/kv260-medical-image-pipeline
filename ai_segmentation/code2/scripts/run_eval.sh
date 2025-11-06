
#!/bin/bash
# UNet评估脚本 - 浮点模型评估

# 设置Python路径和环境变量
export PYTHONPATH=${PWD}/..:${PYTHONPATH}
export W_QUANT=0

# 启动评估
CUDA_VISIBLE_DEVICES='0' python ../test/test.py \
        --data_root '../../data/Chaos/Train_Sets/CT' \
        --input_size 512,512 \
        --weight '../models/ChaosCT_0.9770885167.pth' \
        --quant_mode float
