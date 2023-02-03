---
layout: post
title: OrbMatcher - ORB-SLAM2
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

特征匹配模块，完成

1. 特征层面相似度计算
2. 模块层面的： 地图点 & 关键点的匹配；地图点 & 地图点 匹配；初始化匹配；三角化匹配；融合匹配

## 关键接口

- 描述子距离：`static int DescriptorDistance(const cv::Mat &a, const cv::Mat &b);`
  汉明距离。

- 在 Tracking 和 Loop Closing 的投影匹配：
  ```c++
    int SearchByProjection(Frame &F, 
    const std::vector<MapPoint*> &vpMapPoints, 
    const float th=3);
    int SearchByProjection(Frame &CurrentFrame, 
        const Frame &LastFrame, 
        const float th, const bool bMono);
    int SearchByProjection(Frame &CurrentFrame, 
        KeyFrame* pKF, 
        const std::set<MapPoint*> &sAlreadyFound, 
        const float th, const int ORBdist);
    int SearchByProjection(KeyFrame* pKF,
        cv::Mat Scw, 
        const std::vector<MapPoint*> &vpPoints, 
        std::vector<MapPoint*> &vpMatched, int th);
  ```

- 基于 BOW 匹配：
  ```c++
    int SearchByBoW(KeyFrame *pKF, 
        Frame &F, 
        std::vector<MapPoint*> &vpMapPointMatches);
    int SearchByBoW(KeyFrame *pKF1, 
        KeyFrame* pKF2, 
        std::vector<MapPoint*> &vpMatches12);
  ```

- 初始化匹配：
  ```c++
    int SearchForInitialization(Frame &F1, 
        Frame &F2, 
        std::vector<cv::Point2f> &vbPrevMatched, 
        std::vector<int> &vnMatches12, 
        int windowSize=10);
  ```

- 三角化匹配：
  ```c++
    int SearchForTriangulation(KeyFrame *pKF1, 
        KeyFrame* pKF2, 
        cv::Mat F12,
        std::vector<pair<size_t, size_t> > &vMatchedPairs, 
        const bool bOnlyStereo);
  ```

- 相似变换(sim3)后匹配
  ```c++
    int SearchBySim3(KeyFrame* pKF1, 
        KeyFrame* pKF2, 
        std::vector<MapPoint *> &vpMatches12, 
        const float &s12, 
        const cv::Mat &R12, 
        const cv::Mat &t12, 
        const float th);
  ```

- 融合
  ```c++
    int Fuse(KeyFrame* pKF, 
        const vector<MapPoint *> &vpMapPoints, 
        const float th=3.0);
    int Fuse(KeyFrame* pKF, 
        cv::Mat Scw, 
        const std::vector<MapPoint*> &vpPoints, 
        float th, 
        vector<MapPoint *> &vpReplacePoint);
  ```