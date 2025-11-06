
import os
import sys
from networks.evaluate import evaluate_main_test
import logging
import warnings
warnings.filterwarnings("ignore")
import torch
from torch.utils import data
import numpy as np

sys.path.insert(0,'..')
from networks.UNet import unet
from networks.evaluate import evaluate_main_test
from dataset.datasets import ChaosCTDataValSet as DataSet
import argparse

# 图像均值 (BGR格式)
IMG_MEAN = np.array((104.00698793,116.66876762,122.67891434), dtype=np.float32)

class Configs():
    """预测配置类"""
    def __init__(self):
        parser = argparse.ArgumentParser("2D-UNet-LW for Chaos-CT segmentation")

        # 数据集相关参数
        parser.add_argument('--data_root', type=str, default='../data/', help='数据集路径')
        parser.add_argument('--input_size', type=str, default='512,512', help='输入图像尺寸')
        parser.add_argument('--list_file', type=str, default='dataset/lists/val_img_seg.txt', help='验证集列表文件')
        parser.add_argument('--weight', type=str, default=None, help='模型权重文件路径')
        parser.add_argument('--classes_num', type=int, default=2, help='类别数量')
        parser.add_argument('--ignore_index', type=int, default=255, help='忽略标签索引')
        parser.add_argument('--save_path', type=str, default='submision_test/', help='测试结果保存路径')

        # 训练相关参数
        parser.add_argument('--gpu_id', type=int, default=0, help='GPU设备ID')
        self.parser = parser

    def parse(self):
        """解析命令行参数"""
        args = self.parser.parse_args()
        print(args)
        return args


def get_dataset(input_size, data_dir):
    """获取测试数据集"""
    valloader = DataSet(root = data_dir,crop_size=(512, 512), mean=IMG_MEAN)
    valloader = valloader.__getitem__(root = data_dir)

    return valloader


def main():
    """主函数：执行预测"""
    args = Configs.parse()

    # 加载模型
    model = unet(n_classes=args.classes_num)
    model.load_state_dict(torch.load(args.weight))

    # 遍历测试集中的所有病例
    filelist = os.listdir(args.data_root)
    for file in filelist:
        count = 0
        data_list = os.listdir(os.path.join(args.data_root, file+'/image/'))

        # 处理每个图像
        for item in data_list:
            print('测试 {} 子集，进度: {} / {}'.format(file, count, len(data_list)))
            count += 1
            data_dir = os.path.join(args.data_dir, file+'/image/'+item)
            test_data = get_dataset(input_size = '512,512',data_dir = data_dir)

            # 设置保存路径
            save_path = os.path.join(args.save_path, file)
            if not os.path.exists(save_path):
                os.makedirs(save_path)
            save_name = save_path +item[:-4]

            # 执行预测
            evaluate_main_test(model,test_data, 0, 512, 512, args.classes_num, ignore_label=args.ignore_index, recurrence = 1, save_path=save_name)

if __name__ == '__main__':
    main()
