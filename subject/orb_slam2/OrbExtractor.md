---
layout: post
title: OrbExtractor - ORB-SLAM2
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

ORB 特征抽取器，计算关键点和对应的描述子。

## 关键接口

- 构造函数：
  ```c++
  ORBextractor(int nfeatures, float scaleFactor, int nlevels, int iniThFAST, int minThFAST);
  ```

- 特征抽取：
  ```c++
  void operator()(
      cv::InputArray image, 
      cv::InputArray mask, // 当前实现没用这个
      std::vector<cv::KeyPoint>& keypoints,
      cv::OutputArray descriptors);
  ```

  和 OpenCV 的接口类似。

