
#!/bin/bash
# UNet训练脚本

# 设置Python路径
export PYTHONPATH=${PWD}/..:${PYTHONPATH}

# 启动训练
CUDA_VISIBLE_DEVICES='0' python ../train/train.py \
        --data-set ChaosCT \
        --gpu 0 \
        --data-dir '../../data/Chaos/Train_Sets/CT' \
        --classes-num 2 \
        --input-size 512,512 \
        --random-mirror \
        --lr 1e-2 \
        --weight-decay 5e-4 \
        --batch-size 8 \
        --num-steps 20000 \
        --is-load-imgnet True \
        --ckpt-path '../../ckpt_unet/' \
        --pretrain-model-imgnet '../models/ChaosCT_0.9770885167.pth'                                                                           
