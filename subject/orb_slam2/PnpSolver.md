---
layout: post
title: PnPSolver - ORB-SLAM2
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

PnP Solver，完成 MapPoints(3D) 与 Frame 中关键点(2D) 的匹配。

## 关键接口

- 构造函数：接受帧和一组 MapPoints.
  `PnPsolver(const Frame &F, const vector<MapPoint*> &vpMapPointMatches);`

- 设置 RANSAC 参数：
  ```c++
    void SetRansacParameters(
        double probability = 0.99, 
        int minInliers = 8 , 
        int maxIterations = 300, 
        int minSet = 4, 
        float epsilon = 0.4,
        float th2 = 5.991);
  ```

- 计算位姿和内点：
  `cv::Mat find(vector<bool> &vbInliers, int &nInliers);`

- 迭代计算位姿和内点：
  ```c++
    cv::Mat iterate(int nIterations, 
        bool &bNoMore, 
        vector<bool> &vbInliers, 
        int &nInliers);
  ```

## 关键数据

存储当前位姿、2D 和 3D 点信息。
