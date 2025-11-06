import logging
import os
import sys
import os.path as osp
import torch
from torch.optim import optim
import numpy as np
import torch.nn as nn
import torch.nn.functional as F

sys.path.insert(0,'..')
from networks.UNet import unet
from utils.criterion import Criterion
from networks.evaluate import evaluate_main
from utils.flops_compute import add_flops_counting_methods
from utils.utils import *

# ============================================================================
# 模型构建和训练管理模块
# 提供UNet模型的创建、训练过程管理、学习率调度等功能
# ============================================================================

def compute_flops(model, input=None):
    """计算模型的FLOPs（浮点运算次数）"""
    input = input if input is not None else torch.Tensor(1, 3, 224, 224)
    model = add_flops_counting_methods(model)
    model.eval()
    model.start_flops_count()

    _ = model(input)

    flops = model.compute_average_flops_cost()
    flops = flops/ 1e9
    return np.float(flops)


def warmup_lr(base_lr, it, warmup_iters=500, warmup_factor=1.0/3.0, method='linear'):
    """学习率预热函数"""
    if method == 'constant':
        factor = warmup_factor
    elif method == 'linear':
        alpha = float(it / warmup_iters)
        factor = warmup_factor * (1 -  alpha) + alpha
    lr = base_lr * factor
    return lr

def get_model(args):
    """创建UNet模型并计算FLOPs"""
    h, w = map(int, args.input_size.split(','))
    model = unet(n_classes=2)
    logging.info(model)
    flops = compute_flops(model, input=torch.Tensor(1, 3, h, w))
    logging.info('FLops = {}G with H*W = {} x {}'.format(flops, h, w))
    return model

class NetModel():
    """UNet模型管理类 - 负责模型创建、训练管理和评估"""

    def name(self):
        """返回模型名称"""
        return '2D-UNet(light-weight) on Chaos-CT Dataset'

    def DataParallelModelProcess(self, model, is_eval='train', device='cuda'):
        """处理模型并行化"""
        parallel_model = torch.nn.DataParallel(model)
        if is_eval == 'eval':
            parallel_model.eval()
        elif is_eval == 'train':
            parallel_model.train()
        else:
            raise ValueError('is_eval should be eval or train')
        parallel_model.float()
        parallel_model.to(device)
        return parallel_model

    def DataParallelCriterionProcess(self, criterion, device='cuda'):
        """处理损失函数并行化"""
        criterion.cuda()
        return criterion

    def __init__(self, args):
        """初始化网络模型"""
        self.args = args
        device = args.device

        # 创建和配置学生模型
        student = get_model(args)
        load_S_model(args, student, False)
        print_model_parm_nums(student, 'student_model')
        self.parallel_student = self.DataParallelModelProcess(student, 'train', device)
        self.student = student

        # 配置优化器
        self.solver = optim.SGD([{'params': filter(lambda p: p.requires_grad, self.student.parameters()), 'initial_lr': args.lr}],
                              args.lr, momentum=args.momentum, weight_decay=args.weight_decay)

        # 配置损失函数（类别权重：背景=1，肝脏=12）
        class_weights = [1, 12]
        self.criterion = self.DataParallelCriterionProcess(Criterion(weight=class_weights, ignore_index=255))
        self.loss = 0.0

        # 创建检查点目录
        if not os.path.exists(args.ckpt_path):
            os.makedirs(args.ckpt_path)

    def AverageMeter_init(self):
        """初始化平均指标计算器"""
        self.parallel_top1_train = AverageMeter()
        self.top1_train = AverageMeter()

    def lr_poly(self, base_lr, iter, max_iter, power):
        """多项式学习率衰减策略"""
        return base_lr*((1-float(iter)/max_iter)**(power))

    def adjust_learning_rate(self, base_lr, optimizer, i_iter):
        """调整学习率"""
        args = self.args
        if args.warmup:
            if i_iter < 500:
                lr = warmup_lr(base_lr, i_iter)
            else:
                lr = base_lr*((1-float(i_iter-500)/(args.num_steps-500))**(args.power))
        else:
            lr = self.lr_poly(base_lr, i_iter, args.num_steps, args.power)
        optimizer.param_groups[0]['lr'] = lr
        return lr

    def evalute_model(self, model, loader, gpu_id, h, w, num_classes, ignore_label, whole):
        """评估模型性能"""
        mean_IU, IU_array, dice = evaluate_main(model=model, loader=loader, gpu_id=gpu_id, h=h, w=w, num_classes=num_classes,
                                                ignore_label=ignore_label, whole=whole)
        return mean_IU, IU_array, dice

    def print_info(self, epoch, step):
        """打印训练信息"""
        logging.info('step:{:5d} lr:{:.6f} loss:{:.5f}'.format(
                        step, self.solver.param_groups[-1]['lr'], self.loss))

    def save_ckpt(self, step, acc, name):
        """保存模型检查点"""
        torch.save(self.student.state_dict(), osp.join(self.args.ckpt_path, name+'_'+str(step)+'_'+str(acc)+'.pth'))




