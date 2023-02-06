---
layout: post
title: Sim3Solver - ORB-SLAM2
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

Sim3 Solver 用与 Loop Closing 时求解相似变换。

## 关键接口

- 构造函数：接受 2 个关键帧和一堆 Map Points. 如果有尺度变换，则 FixScale 为真。
  ```c++
    Sim3Solver(
        KeyFrame* pKF1, 
        KeyFrame* pKF2, 
        const std::vector<MapPoint*> &vpMatched12, 
        const bool bFixScale = true);
  ```

- 设置 RANSAC 参数：
  ```c++
    void SetRansacParameters(
        double probability = 0.99, 
        int minInliers = 6 , 
        int maxIterations = 300);
  ```

- 求解：
  ```c++
    cv::Mat find(std::vector<bool> &vbInliers12, int &nInliers);

    cv::Mat iterate(int nIterations, 
        bool &bNoMore, 
        std::vector<bool> &vbInliers, 
        int &nInliers);
  ```

