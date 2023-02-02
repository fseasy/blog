---
layout: post
title: Converter - ORB-SLAM2
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

工具类，包含了各种数据格式转换操作（各种第三方库不是一套底层啊）。

### 关键接口

- 描述子数组转换： `std::vector<cv::Mat> toDescriptorVector(const cv::Mat &Descriptors);`

- `cv::Mat`, `g2o::Sim3` $\to$ `g2o::SE3Quat`
- `g2o::SE3Quat`, `g2o::Sim3` $\to$ `cv::Mat`; Eigen 类型的 $T, R, t, R+t$ $\to$ `cv::Mat`
- `cv::Mat` 和 `cv::Point3f` 到 Eigen 的 `Vector3d`; `cv::Mat` 到 Eigen `Matrix3d`
- `cv::Mat` 到 `std::vector<float>` 的四元数