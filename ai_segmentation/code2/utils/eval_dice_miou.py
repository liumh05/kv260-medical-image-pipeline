
import argparse
import scipy
from scipy import ndimage
import cv2
import numpy as np
import sys
import os
from PIL import Image 

def get_arguments():
    """Parse all the arguments.

    Returns:
      A list of parsed arguments.
    """
    parser = argparse.ArgumentParser(description="Evaluation script")

    parser.add_argument("--gt_path", type=str,
                        help="Path to the directory containing the annotation.")

    parser.add_argument("--pred_path", type=str,
                        help="Path to the directory containing the prediction.")
    parser.add_argument("--num-classes", type=int, default=2,
                        help="Number of classes to predict.")
    return parser.parse_args()

def general_dice(y_true, y_pred):
    result = []

    if y_true.sum() == 0:
        if y_pred.sum() == 0:
            return 1
        else:
            return 0

    for instrument_id in set(y_true.flatten()):
        result += [dice(y_true == instrument_id, y_pred == instrument_id)]

    return np.mean(result)


def general_jaccard(y_true, y_pred):
    result = []

    if y_true.sum() == 0:
        if y_pred.sum() == 0:
            return 1
        else:
            return 0

    for instrument_id in set(y_true.flatten()):
        result += [jaccard(y_true == instrument_id, y_pred == instrument_id)]

    return np.mean(result)


def jaccard(y_true, y_pred):
    intersection = (y_true * y_pred).sum()
    union = y_true.sum() + y_pred.sum() - intersection
    return (intersection ) / (union )


def dice(y_true, y_pred):
    return (2 * (y_true * y_pred).sum() ) / (y_true.sum() + y_pred.sum())



def eval(num_classes, pred_path, gt_path):
    result_dice = []
    result_jaccard = []

    categories = os.listdir(args.gt_path)
    for c in categories:
        c_items = [name for name in os.listdir(os.path.join(gt_path, c, 'Ground'))]
        for it in c_items:
            pred_file = os.path.join(pred_path, c, it.replace('liver_GT','Liver_image'))
            gt_file = os.path.join(gt_path, c, 'Ground', it)
            seg_gt = np.array(Image.open(gt_file))
            seg_gt[seg_gt==255]=1
            seg_pred = np.array(Image.open(pred_file))

            result_dice += [general_dice(seg_gt, seg_pred)]
            result_jaccard += [general_jaccard(seg_gt, seg_pred)]
             

    print('Dice = ', np.mean(result_dice))
    print('Jaccard = ', np.mean(result_jaccard))



if __name__ == '__main__':
    args = get_arguments()
    eval(args.num_classes, args.pred_path, args.gt_path)
