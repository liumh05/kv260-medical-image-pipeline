#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CT图像肝脏分割推理脚本
使用训练好的UNet模型对CT测试集进行推理

使用方法:
    python inference.py --model_path ../float/ChaosCT_0.9770885167.pth --test_dir ../data/Chaos/Test_Sets/CT --output_dir ./results
"""

import os
import sys
import argparse
import logging
import warnings
warnings.filterwarnings("ignore")

import cv2
import numpy as np
import torch
import torch.nn.functional as F
from tqdm import tqdm
from pathlib import Path
import json

# 添加当前目录到Python路径
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from networks.UNet import unet

# 设置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# 预处理参数
IMG_MEAN = np.array([104.00698793, 116.66876762, 122.67891434], dtype=np.float32)
INPUT_SIZE = (512, 512)

class CTInference:
    def __init__(self, model_path, device='cuda'):
        """
        初始化推理器

        Args:
            model_path: 模型权重文件路径
            device: 推理设备 ('cuda' 或 'cpu')
        """
        self.device = torch.device(device if torch.cuda.is_available() else 'cpu')
        self.model = self.load_model(model_path)
        logger.info(f"模型加载完成，使用设备: {self.device}")

    def load_model(self, model_path):
        """
        加载训练好的模型

        Args:
            model_path: 模型权重文件路径

        Returns:
            加载好的模型
        """
        if not os.path.exists(model_path):
            raise FileNotFoundError(f"模型文件不存在: {model_path}")

        # 创建模型
        model = unet(n_classes=2)

        # 加载权重
        try:
            checkpoint = torch.load(model_path, map_location=self.device)
            if 'state_dict' in checkpoint:
                model.load_state_dict(checkpoint['state_dict'])
            else:
                model.load_state_dict(checkpoint)
            logger.info("模型权重加载成功")
        except Exception as e:
            logger.error(f"模型权重加载失败: {e}")
            raise

        model.to(self.device)
        model.eval()
        return model

    def preprocess_image(self, image_path):
        """
        预处理单张CT图像

        Args:
            image_path: 图像文件路径

        Returns:
            预处理后的图像tensor [1, 3, 512, 512]
        """
        # 读取图像 (BGR格式)
        image = cv2.imread(image_path, cv2.IMREAD_COLOR)
        if image is None:
            raise ValueError(f"无法读取图像: {image_path}")

        # 记录原始尺寸
        original_size = image.shape[:2]  # (H, W)

        # 调整到512x512
        image = cv2.resize(image, INPUT_SIZE, interpolation=cv2.INTER_AREA)

        # 均值减法
        image = np.asarray(image, np.float32)
        image -= IMG_MEAN

        # 维度转换 HWC -> CHW
        image = image.transpose((2, 0, 1))

        # 添加批次维度
        image = image[np.newaxis, :, :, :]  # [1, 3, 512, 512]

        # 转换为tensor
        image = torch.from_numpy(image).to(self.device)

        return image, original_size

    def postprocess_output(self, output, original_size):
        """
        后处理模型输出

        Args:
            output: 模型原始输出 [1, 2, 512, 512]
            original_size: 原始图像尺寸 (H, W)

        Returns:
            分割掩码，调整到原始尺寸
        """
        # 取argmax得到类别标签
        if isinstance(output, torch.Tensor):
            output = output.detach().cpu().numpy()

        # argmax across channel dimension
        mask = np.argmax(output, axis=1)[0]  # [512, 512]

        # 调整回原始尺寸
        mask = cv2.resize(mask.astype(np.uint8),
                         (original_size[1], original_size[0]),
                         interpolation=cv2.INTER_NEAREST)

        return mask

    def predict_single_image(self, image_path):
        """
        对单张图像进行推理

        Args:
            image_path: 图像文件路径

        Returns:
            分割掩码
        """
        # 预处理
        input_tensor, original_size = self.preprocess_image(image_path)

        # 推理
        with torch.no_grad():
            output = self.model(input_tensor)

        # 后处理
        mask = self.postprocess_output(output, original_size)

        return mask

    def find_images_in_directory(self, test_dir):
        """
        在目录中查找所有图像文件

        Args:
            test_dir: 测试目录路径

        Returns:
            图像文件路径列表
        """
        image_extensions = {'.png', '.jpg', '.jpeg', '.bmp', '.tiff', '.tif'}
        image_files = []

        test_path = Path(test_dir)
        if not test_path.exists():
            raise FileNotFoundError(f"测试目录不存在: {test_dir}")

        # 递归查找所有图像文件
        for ext in image_extensions:
            image_files.extend(test_path.rglob(f'*{ext}'))
            image_files.extend(test_path.rglob(f'*{ext.upper()}'))

        # 转换为字符串路径并排序
        image_files = [str(f) for f in image_files]
        image_files.sort()

        logger.info(f"在 {test_dir} 中找到 {len(image_files)} 张图像")
        return image_files

    def predict_directory(self, test_dir, output_dir, save_overlay=True):
        """
        对整个目录的图像进行推理

        Args:
            test_dir: 测试目录路径
            output_dir: 输出目录路径
            save_overlay: 是否保存叠加可视化结果
        """
        # 创建输出目录
        os.makedirs(output_dir, exist_ok=True)
        if save_overlay:
            overlay_dir = os.path.join(output_dir, 'overlays')
            os.makedirs(overlay_dir, exist_ok=True)

        # 查找所有图像文件
        image_files = self.find_images_in_directory(test_dir)

        if not image_files:
            logger.warning(f"在 {test_dir} 中未找到图像文件")
            return

        # 推理结果统计
        results = {
            'total_images': len(image_files),
            'processed_images': 0,
            'failed_images': 0,
            'failed_files': [],
            'output_dir': output_dir
        }

        # 逐张处理
        for image_path in tqdm(image_files, desc="推理进度"):
            try:
                # 推理
                mask = self.predict_single_image(image_path)

                # 保存分割掩码
                rel_path = os.path.relpath(image_path, test_dir)
                mask_filename = os.path.splitext(rel_path)[0] + '_mask.png'
                mask_path = os.path.join(output_dir, mask_filename)

                # 确保输出目录存在
                os.makedirs(os.path.dirname(mask_path), exist_ok=True)

                # 保存掩码
                cv2.imwrite(mask_path, mask * 255)  # 乘以255使其可见

                # 保存叠加可视化
                if save_overlay:
                    overlay = self.create_overlay(image_path, mask)
                    overlay_filename = os.path.splitext(rel_path)[0] + '_overlay.png'
                    overlay_path = os.path.join(overlay_dir, overlay_filename)
                    os.makedirs(os.path.dirname(overlay_path), exist_ok=True)
                    cv2.imwrite(overlay_path, overlay)

                results['processed_images'] += 1

            except Exception as e:
                logger.error(f"处理图像失败 {image_path}: {e}")
                results['failed_images'] += 1
                results['failed_files'].append(image_path)

        # 保存推理统计信息
        stats_path = os.path.join(output_dir, 'inference_stats.json')
        with open(stats_path, 'w', encoding='utf-8') as f:
            json.dump(results, f, indent=2, ensure_ascii=False)

        logger.info(f"推理完成! 成功: {results['processed_images']}, 失败: {results['failed_images']}")
        logger.info(f"结果保存在: {output_dir}")

    def create_overlay(self, image_path, mask):
        """
        创建分割结果叠加可视化

        Args:
            image_path: 原始图像路径
            mask: 分割掩码

        Returns:
            叠加图像
        """
        # 读取原始图像
        original_image = cv2.imread(image_path, cv2.IMREAD_COLOR)

        # 创建彩色掩码 (红色表示肝脏区域)
        colored_mask = np.zeros_like(original_image)
        colored_mask[mask == 1] = [0, 0, 255]  # 红色 (BGR格式)

        # 创建叠加效果
        overlay = cv2.addWeighted(original_image, 0.7, colored_mask, 0.3, 0)

        return overlay


def main():
    parser = argparse.ArgumentParser(description='CT图像肝脏分割推理脚本')

    parser.add_argument('--model_path', type=str, required=True,
                        help='模型权重文件路径 (例如: ../float/ChaosCT_0.9770885167.pth)')
    parser.add_argument('--test_dir', type=str, required=True,
                        help='测试图像目录路径 (例如: ../data/Chaos/Test_Sets/CT)')
    parser.add_argument('--output_dir', type=str, default='./results',
                        help='输出结果目录路径 (默认: ./results)')
    parser.add_argument('--device', type=str, default='cuda', choices=['cuda', 'cpu'],
                        help='推理设备 (默认: cuda)')
    parser.add_argument('--save_overlay', action='store_true',
                        help='是否保存叠加可视化结果')

    args = parser.parse_args()

    # 检查输入参数
    if not os.path.exists(args.model_path):
        logger.error(f"模型文件不存在: {args.model_path}")
        return

    if not os.path.exists(args.test_dir):
        logger.error(f"测试目录不存在: {args.test_dir}")
        return

    try:
        # 创建推理器
        inference = CTInference(args.model_path, args.device)

        # 执行推理
        inference.predict_directory(args.test_dir, args.output_dir, args.save_overlay)

    except Exception as e:
        logger.error(f"推理过程中发生错误: {e}")
        raise


if __name__ == '__main__':
    main()