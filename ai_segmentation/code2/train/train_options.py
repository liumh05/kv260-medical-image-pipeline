import argparse
import torch
import logging
import os
import sys

# 添加上级目录到路径
sys.path.insert(0,'..')
from utils.utils import *

def str2bool(v):
    """字符串转布尔值"""
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')

class TrainOptions():
    """训练参数配置类"""
    def initialize(self):
        parser = argparse.ArgumentParser(description='UNet training options')

        # 模型相关参数
        parser.add_argument('--model_path', default=None, type=str, help='模型路径')
        parser.add_argument('--normlayer', default='BN', type=str, help='归一化层类型')
        parser.add_argument('--index', default=1, type=int, help='索引号')
        parser.add_argument('--data-set', default='cityscape', type=str, help='数据集名称')
        parser.add_argument('--data-dir', type=str, default='/scratch/workspace/wangli/Dataset/', help='数据集路径')
        parser.add_argument('--ignore-index', default=255, type=int, help='忽略标签索引')
        parser.add_argument('--classes-num', default=19, type=int, help='类别数量')
        parser.add_argument('--loss-w', default=1.0, type=float, help='损失权重')

        # 训练相关参数
        parser.add_argument('--resume', default='True', type=str2bool, help='是否恢复训练')
        parser.add_argument('--ckpt-path', default='', type=str, help='检查点路径')
        parser.add_argument("--batch-size", type=int, default=8, help='批次大小')
        parser.add_argument('--start_epoch', default=0, type=int, help='起始epoch')
        parser.add_argument('--parallel', default='True', type=str, help='是否并行训练')
        parser.add_argument("--input-size", type=str, default='512,512', help='输入尺寸')
        parser.add_argument("--is-training", action="store_true", help='是否为训练模式')
        parser.add_argument("--momentum", type=float, default=0.9, help='动量参数')
        parser.add_argument("--num-steps", type=int, default=40000, help='训练步数')
        parser.add_argument("--save-steps", type=int, default=10000, help='保存步数')
        parser.add_argument("--power", type=float, default=0.9, help='学习率衰减幂次')
        parser.add_argument("--random-mirror", action="store_true", help='随机镜像')
        parser.add_argument("--warmup", action="store_true", help='学习率预热')
        parser.add_argument("--random-scale", action="store_true", help='随机缩放')
        parser.add_argument("--weight-decay", type=float, default=1.0e-4, help='权重衰减')
        parser.add_argument("--gpu", type=str, default='None', help='GPU设备')
        parser.add_argument("--recurrence", type=int, default=1, help='循环次数')
        parser.add_argument("--last-step", type=int, default=0, help='最后一步')
        parser.add_argument("--is-load-imgnet", type=str2bool, default='True', help='是否加载ImageNet预训练权重')
        parser.add_argument("--pretrain-model-imgnet", type=str, default='None', help='ImageNet预训练模型路径')
        parser.add_argument("--lr", type=float, default=1e-2, help='学习率')

        args = parser.parse_args()

        # 设置日志路径
        args.log_path = os.path.join(args.ckpt_path, 'logs')
        log_init(args.log_path, args.data_set)

        # 设置设备
        args.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        args.gpu_num = len(args.gpu.split(','))

        # 创建日志目录
        if not os.path.exists(args.log_path):
            os.makedirs(args.log_path)

        return args