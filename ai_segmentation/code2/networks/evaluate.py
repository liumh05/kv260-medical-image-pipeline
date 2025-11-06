
import argparse
import scipy
from scipy import ndimage
import cv2
import numpy as np
import sys
import torch
from torch.autograd import Variable
import torchvision.models as models
import torch.nn.functional as F
from torch.utils import data
from collections import OrderedDict
import os
import scipy.ndimage as nd
from math import ceil
from PIL import Image as PILImage
import torch.nn as nn

# 图像均值 (BGR格式)
IMG_MEAN = np.array((104.00698793,116.66876762,122.67891434), dtype=np.float32)

def predict_whole(net, image, tile_size, device, recurrence):
    """整个图像预测函数"""
    image = torch.from_numpy(image)
    interp = nn.Upsample(size=tile_size, mode='bilinear', align_corners=True)
    prediction = net(image.to(device))
    if isinstance(prediction, list):
        prediction = prediction[0]
    prediction = interp(prediction).cpu().data[0].numpy().transpose(1,2,0)
    return prediction

def predict_multiscale(net, image, tile_size, scales, classes, device, flip_evaluation, recurrence):
    """多尺度预测函数 - 通过不同尺度查看图像来预测"""
    image = image.data
    N_, C_, H_, W_ = image.shape
    full_probs = np.zeros((H_, W_, classes))
    for scale in scales:
        scale = float(scale)
        scale_image = ndimage.zoom(image, (1.0, 1.0, scale, scale), order=1, prefilter=False)
        scaled_probs = predict_whole(net, scale_image, tile_size, device, recurrence)
        full_probs += scaled_probs
    full_probs /= len(scales)
    return full_probs

def predict_multiscale_test(net, image, tile_size, scales, classes, device, flip_evaluation, recurrence):
    """测试多尺度预测函数"""
    N_, C_, H_, W_ = 1,3,512,512
    full_probs = np.zeros((H_, W_, classes))
    for scale in scales:
        scale = float(scale)
        scale_image = ndimage.zoom(image, (1.0, 1.0, scale, scale), order=1, prefilter=False)
        scaled_probs = predict_whole(net, scale_image, tile_size, device, recurrence)
        full_probs += scaled_probs
    full_probs /= len(scales)
    return full_probs

def get_confusion_matrix(gt_label, pred_label, class_num):
    """计算混淆矩阵"""
    index = (gt_label * class_num + pred_label).astype('int32')
    label_count = np.bincount(index)
    confusion_matrix = np.zeros((class_num, class_num))

    for i_label in range(class_num):
        for i_pred_label in range(class_num):
            cur_index = i_label * class_num + i_pred_label
            if cur_index < len(label_count):
                confusion_matrix[i_label, i_pred_label] = label_count[cur_index]

    return confusion_matrix



def evaluate_main(model,loader, gpu_id, h, w, num_classes, ignore_label=255, whole = False, device='cuda', recurrence = 1, type = 'val'):
    """主评估函数 - 创建模型并开始评估过程"""
    input_size = (h, w)
    model.eval()
    model.to(device)

    confusion_matrix = np.zeros((num_classes,num_classes))

    for index, batch in enumerate(loader):
        print('处理图像: {}/{}'.format(index, len(loader)))
        image, label, size, name = batch
        size = size[0].numpy()
        with torch.no_grad():
            output = predict_multiscale(model, image, input_size, [1.0], num_classes, device, False, recurrence)
        seg_pred = np.asarray(np.argmax(output, axis=2), dtype=np.uint8)
        seg_gt = np.asarray(label[0].numpy()[:size[0],:size[1]], dtype=np.int)
        ignore_index = seg_gt != ignore_label
        seg_gt = seg_gt[ignore_index]
        seg_pred = seg_pred[ignore_index]
        confusion_matrix += get_confusion_matrix(seg_gt, seg_pred, num_classes)

    pos = confusion_matrix.sum(1)
    res = confusion_matrix.sum(0)
    tp = np.diag(confusion_matrix)
    IU_array = (tp / np.maximum(1.0, pos + res - tp))
    mean_IU = IU_array.mean()
    Dice_array = 2.0 * (tp / np.maximum(1.0, pos + res))
    dice = Dice_array.mean()
    return mean_IU, IU_array, dice

def evaluate_main_test(model,image, gpu_id, h, w, num_classes, ignore_label=255, device='cuda', recurrence = 1, save_path = 'output'):
    """测试评估函数 - 创建模型并开始评估测试过程"""
    input_size = (h, w)

    model.eval()
    model.to(device)

    with torch.no_grad():
        output = predict_multiscale_test(model, image, input_size, [1.0], num_classes, False, recurrence)
    seg_pred = np.asarray(255*np.argmax(output, axis=2), dtype=np.uint8)
    output_im = PILImage.fromarray(seg_pred).convert('L')
    output_im.save(save_path+'.png')
