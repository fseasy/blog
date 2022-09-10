---
layout: post
title: VSLAM 代码笔记
date: 2022-09-10
categories: 技术 
tags: vslam code-notes
---
> 学习 vslam 过程中的一些代码记录。

## 1 相机与图像

1.  像素点坐标 $p$ 到归一化平面上的3d坐标 $P$

    按 $p = K P$ 看， $P = K^{-1} p $, 但没必要这样算，直接按照求 p 的逆运算即可：

    - 先减去中心偏移
    - 再除以缩放系数(focal)

    ```c++
    cv::Mat _pixel_pnt2camera_3d(const cv::Point2f& p, const cv::Mat& camera_intrinsic) {
        auto K_ = static_cast<cv::Mat_<double>>(camera_intrinsic);
        double cx = K_(0, 2);
        double cy = K_(1, 2);
        double fx = K_(0, 0);
        double fy = K_(1, 1);
        return (cv::Mat_<double>(3, 1) << 
            (p.x - cx) / fx,
            (p.y - cy) / fy,
            1
        );
    };
    ```