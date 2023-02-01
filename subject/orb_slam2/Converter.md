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

- `cv::Mat`, `g2o::Sim3` $\Rightarrow$ `g2o::SE3Quat`