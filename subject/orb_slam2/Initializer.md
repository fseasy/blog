---
layout: post
title: Initializer - ORB-SLAM2
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

单目初始化器。

## 关键接口

- 构造函数：`Initializer(const Frame &ReferenceFrame, float sigma = 1.0, int iterations = 200);`
  Reference Frame 是普通帧，这时还没有关键帧形成的。
  iterations 是 RANSAC 的最大迭代轮次。

- 初始化逻辑：给定了当前帧和与参考帧的匹配关系。
  
  ```cpp
  bool Initialize(const Frame &CurrentFrame, const vector<int> &vMatches12,
        cv::Mat &R21, cv::Mat &t21, 
        vector<cv::Point3f> &vP3D, 
        vector<bool> &vbTriangulated);
  ```
