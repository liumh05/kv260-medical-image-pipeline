import torch
import torch.nn as nn
from torch.nn import functional as F

class Criterion(nn.Module):
    def __init__(self, ignore_index=255, weight=None, use_weight=True, reduce=True):
        super(Criterion, self).__init__()
        if weight is not None:
            class_wts = torch.ones(len(weight))
            for i in range(len(weight)):
                class_wts[i] = weight[i]
        else:
            class_wts = None
        self.ignore_index = ignore_index
        self.criterion = torch.nn.CrossEntropyLoss(weight=class_wts, ignore_index=ignore_index, reduce=reduce)

    def forward(self, preds, target):
        h, w = target.size(1), target.size(2)
        scale_pred = F.upsample(input=preds, size=(h, w), mode='bilinear', align_corners=True)
        loss = self.criterion(scale_pred, target)
        return loss

