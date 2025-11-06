
import SimpleITK as sitk
import numpy as np
import cv2
import pdb
import os

def convert_from_dicom_to_jpg(img,low_window,high_window,save_path):
    """将DICOM图像转换为JPG格式"""
    lungwin = np.array([low_window*1.,high_window*1.])
    newimg = (img-lungwin[0])/(lungwin[1]-lungwin[0])
    newimg = (newimg*255).astype('uint8')
    cv2.imwrite(save_path, newimg, [int(cv2.IMWRITE_JPEG_QUALITY), 100])

if __name__ == '__main__':
    """主函数：批量转换DICOM图像为PNG格式"""
    # 训练集和测试集的病例ID
    # ids = [11, 12, 13, 15, 17, 20, 3, 31, 32, 33, 34, 35, 36, 37, 38, 39, 4, 40, 7, 9]

    mode = 'train'  # 设置模式：'train' 或 'test'

    if mode == 'train':
        data_path = 'Train_Sets'
        ids = [1,2,5,6,8,10,14,16,18,19,21,22,23,24,25,26,27,28,29,30]  # 训练集病例ID
    elif mode == 'test':
        data_path = 'Test_Sets'
        ids = [3,4,7,9,11,12,13,15,17,20,31,32,33,34,35,36,37,38,39,40]  # 测试集病例ID

    # 遍历每个病例
    for index in ids:
        print('正在处理病例:', index)
        count = 0
        dcm_image_root = os.path.join('data/Chaos', data_path, 'CT', str(index), 'DICOM_anon/')
        save_image_root = os.path.join('data/Chaos', data_path,'CT', str(index), 'image/')

        # 创建保存目录
        if not os.path.exists(save_image_root):
            os.makedirs(save_image_root)

        # 遍历DICOM文件
        for name in os.listdir(dcm_image_root):
            print('  处理文件:', index, name)
            dcm_image_path = os.path.join(dcm_image_root, name)
            out_name = 'Liver_image_'+name[:-4].split(',')[0][-3:]+'.png'
            output_jpg_path = os.path.join(save_image_root, out_name)

            # 读取DICOM文件
            ds_array = sitk.ReadImage(dcm_image_path)
            img_array = sitk.GetArrayFromImage(ds_array)

            # 去除异常值
            img_array[img_array > 10000]=0
            shape = img_array.shape
            img_array = np.reshape(img_array, (shape[1], shape[2]))

            # 获取图像强度范围
            high = np.max(img_array)
            low = np.min(img_array)

            # 转换并保存
            convert_from_dicom_to_jpg(img_array, low, high, output_jpg_path)

    print('转换完成')

