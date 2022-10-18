---
layout: post
title: VSLAM 代码笔记
date: 2022-09-10
categories: 技术 
tags: vslam code-notes
---
> 学习 vslam 过程中的一些代码记录。

## OpenCV 基础

1. `cv::Mat` 用起来方便，但还是挺容易有坑的！最大的问题，就是 `.at<T>`, 如果 `T` 不是实际类型，运算结果会错误，但程序多半不会挂…… 譬如， `cv::recoverPose` api 返回的 `R`, 类型是 `CV_64F`, 即 `double`, 如果用 `.at<float>`, 结果就错了。这要求我们必须得知道输出的类型具体是啥。然而，api 上不一定会标注出来…… 

   目前明确的类型：多视图几何相关的 api 里：相机矩阵K，一般要声明为`double`; R、t 都是 `double` 的；传给 `cv::triangulatePoints` 必须传入 `float`. 

   尽量少用 `.at`, 可以使用 `.copyTo` 来赋值，这类函数，内部一般会做好类型转换，不会出问题。

## 相机与图像

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

    > 注1：经过实际测试，上述做法，和 $K^{-1} p$ 结果一致。
    
    > 注2：代码中 `static_cast<cv::Mat_<double>>(camera_intrinsic)`，其实是调用了 `cv::Mat_` 的构造函数： `Mat_ (const Mat &m)`, 这个包含了拷贝或转换逻辑，是有代价的！尽量不应该这么做。