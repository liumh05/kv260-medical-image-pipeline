import logging
import sys
import warnings
warnings.filterwarnings("ignore")
import torch
from torch.utils import data
import numpy as np

# 添加上级目录到路径
sys.path.insert(0,'..')
from dataset.datasets import ChaosCTDataSet as DataSet
from utils.train_options import TrainOptions
from networks.build_model import NetModel

# 图像均值 (BGR格式)
IMG_MEAN = np.array((104.00698793,116.66876762,122.67891434), dtype=np.float32)

def main():
    """主训练函数"""
    args = TrainOptions().initialize()
    model = NetModel(args)
    trainloader, valloader, save_steps, H, W = get_dataset(args, 'ChaosCT')
    batch = iter(trainloader)

    max_dice = 0  # 记录最佳Dice分数
    for iteration in range(0, args.num_steps):
        # 调整学习率
        model.adjust_learning_rate(args.lr, model.solver, iteration)
        model.solver.zero_grad()

        # 前向传播
        inputs, labels = set_input(next(batch))
        pred = model.parallel_student.train()(inputs)
        loss = model.criterion(pred, labels)
        model.loss = loss.item()

        # 反向传播
        loss.backward()
        model.solver.step()

        # 打印训练日志
        if iteration % 20 == 0:
            logging.info('iteration:{:5d} lr:{:.6f} loss:{:.5f}'.format( \
                        iteration, model.solver.param_groups[-1]['lr'], model.loss))

        # 验证模型并保存最佳权重
        if (iteration > 1) and ((iteration % save_steps == 0) and (iteration > args.num_steps - 2000)) or (iteration == args.num_steps - 1):
            mean_IU, IU_array, Dice = model.evalute_model(model.student, valloader, args.gpu, H, W, args.classes_num, args.ignore_index, True)
            logging.info('[验证 {}x{}] mean_IU:{:.6f} Dice:{:.6f}'.format(H, W, mean_IU, Dice))
            if Dice > max_dice:
                max_dice = Dice
                model.save_ckpt(iteration, Dice, 'ChaosCT')
                logging.info('保存最佳模型: iteration {} Dice:{:.6f}'.format(iteration, max_dice))

def set_input(data):
    """设置输入数据，移动到GPU"""
    images, labels, _, _ = data
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    images = images.to(device)
    labels = labels.long().to(device)
    return images, labels

def get_dataset(args, data_set):
    """获取训练和验证数据集"""
    h, w = map(int, args.input_size.split(','))

    # 训练集数据加载器
    trainloader = data.DataLoader(
        DataSet(root=args.data_dir,
                list_path='./dataset/lists/train_img_seg.txt',
                max_iters=args.num_steps*args.batch_size,
                crop_size=(h, w),
                mean=IMG_MEAN,
                scale=args.random_scale,
                mirror=args.random_mirror,
                ignore_label=args.ignore_index),
        batch_size=args.batch_size,
        shuffle=False,
        num_workers=4,
        pin_memory=True,
        drop_last=True
    )

    # 验证集数据加载器
    valloader = data.DataLoader(
        DataSet(root=args.data_dir,
                list_path='./dataset/lists/val_img_seg.txt',
                crop_size=(512, 512),
                mean=IMG_MEAN,
                scale=True,
                mirror=False,
                ignore_label=args.ignore_index),
        batch_size=1,
        shuffle=False,
        pin_memory=True
    )

    save_steps = int(479/args.batch_size)  # 保存步数
    H, W = 512, 512  # 输入尺寸

    return trainloader, valloader, save_steps, H, W

if __name__ == '__main__':
    main()