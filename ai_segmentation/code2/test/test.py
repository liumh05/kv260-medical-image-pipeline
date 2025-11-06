import os
import sys
import warnings
import argparse
import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.utils import data
import numpy as np

# 量化相关导入
if os.environ.get("W_QUANT") == '1':
    from pytorch_nndct.apis import torch_quantizer, dump_xmodel

# 忽略警告信息
warnings.filterwarnings("ignore")

# 添加路径并导入模块
sys.path.insert(0,'..')
from dataset.datasets import ChaosCTDataValSet as DataSet
from networks.evaluate import evaluate_main
from networks.UNet import unet

# 图像均值 (BGR格式)
IMG_MEAN = np.array((104.00698793,116.66876762,122.67891434), dtype=np.float32)

class Configs():
    """测试配置类"""
    def __init__(self):
        parser = argparse.ArgumentParser("2D-UNet-LW for Chaos-CT segmentation")

        # 数据相关参数
        parser.add_argument('--data_root', type=str, default='../data', help='数据根目录')
        parser.add_argument('--input_size', type=str, default='512,512', help='输入图像尺寸')
        parser.add_argument('--list_file', type=str, default='dataset/lists/val_img_seg.txt', help='验证集列表文件')
        parser.add_argument('--weight', type=str, default=None, help='模型权重文件路径')
        parser.add_argument('--classes_num', type=int, default=2, help='类别数量')
        parser.add_argument('--ignore_index', type=int, default=255, help='忽略标签索引')

        # 设备相关参数
        parser.add_argument('--gpu', type=int, default=0, help='GPU设备ID')
        parser.add_argument('--device', default='gpu', choices=['gpu', 'cpu'], help='运行设备')

        # 量化相关参数
        parser.add_argument('--quant_dir', type=str, default='quantize_result', help='量化结果保存目录')
        parser.add_argument('--quant_mode', default='calib', choices=['float', 'calib', 'test'], help='量化模式')
        parser.add_argument('--finetune', dest='finetune', action='store_true', help='是否微调')
        parser.add_argument('--dump_xmodel', dest='dump_xmodel', action='store_true', help='是否导出XModel')

        self.parser = parser

    def parse(self):
        """解析命令行参数"""
        args = self.parser.parse_args()
        return args

class Criterion(nn.Module):
    """损失函数类"""
    def __init__(self, ignore_index=255, weight=None, use_weight=True, reduce=True):
        super(Criterion, self).__init__()
        self.ignore_index = ignore_index
        self.criterion = torch.nn.CrossEntropyLoss(weight=weight, ignore_index=ignore_index, reduce=reduce)

    def forward(self, preds, target):
        """前向传播计算损失"""
        h, w = target.size(1), target.size(2)
        # 上采样预测结果到目标尺寸
        scale_pred = F.interpolate(input=preds, size=(h, w), mode='bilinear', align_corners=True)
        loss = self.criterion(scale_pred, target)
        return loss


def main():
    args = Configs().parse()

    if args.dump_xmodel:
        args.device = 'cpu'

    device = torch.device("cpu" if args.device == 'cpu' else "cuda")

    model = unet(n_classes=2).to(device)
    if os.path.isfile(args.weight):
        weight_state_dict = torch.load(args.weight, map_location=device)
        model_state_dict = model.state_dict()
        overlap_state_dict = {k:v for k,v in weight_state_dict.items() if k in model_state_dict}
        model_state_dict.update(overlap_state_dict)
        model.load_state_dict(model_state_dict)
        print("Load weights successfully")
    else:
        print('Cannot find the checkpoint.')
        exit(-1)

    valloader, H, W = get_dataset(args, 'ChaosCT')
    input = torch.randn([1, 3, H, W])

    if args.quant_mode == 'float':
        quant_model = model
    else:
        quantizer = torch_quantizer(args.quant_mode, model, (input), output_dir=args.quant_dir, device=device)
        quant_model = quantizer.quant_model

    criterion = Criterion(ignore_index=255, weight=None, use_weight=False, reduce=True)
    loss_fn = criterion.to(device)

    if args.quant_mode == 'calib' and args.finetune:
        ft_loader = valloader
        quantizer.finetune(evaluate_main, (quant_model, ft_loader, loss_fn))

    mean_IU, IU_array, Dice = evaluate_main(quant_model, valloader, 0, H, W, args.classes_num, args.ignore_index, True, device)
    print('[val with {}x{}] mean_IU:{:.6f} Dice:{}'.format(H, W, mean_IU, Dice))

    if args.quant_mode == 'calib':
        quantizer.export_quant_config()
    if args.quant_mode == 'test' and args.dump_xmodel:
        dump_xmodel(args.quant_dir, deploy_check=True)

def get_dataset(args, data_set):
    h, w = map(int, args.input_size.split(','))
    valloader = data.DataLoader(DataSet(root=args.data_root, list_path='./dataset/lists/val_img_seg.txt', \
                      crop_size=(h, w), mean=IMG_MEAN, scale=False, mirror=False, ignore_label=args.ignore_index), \
                      batch_size=1, shuffle=False, pin_memory=True)

    return valloader, h, w

if __name__ == '__main__':
    main()

