---
layout: post
title: Optimizer - ORB-SLAM2
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

(Bundle Adjustment) 优化器工具类，把所有的优化器接口都集中到这里了。

## 主要接口

全部为静态成员函数。

- 基础 BA, 给定关键帧和地图点。
  ```cpp
    void static BundleAdjustment(
        const std::vector<KeyFrame*> &vpKF, 
        const std::vector<MapPoint*> &vpMP,
        int nIterations = 5, 
        bool *pbStopFlag=NULL, 
        const unsigned long nLoopKF=0,
        const bool bRobust = true);
  ```
  
- Global BA, 给定地图。
  ```cpp
  void static GlobalBundleAdjustemnt(
        Map* pMap, 
        int nIterations=5, 
        bool *pbStopFlag=NULL,
        const unsigned long nLoopKF=0, 
        const bool bRobust = true);
  ```

- 局部 BA, 给定插入的关键帧和地图。
  ```cpp
  void static LocalBundleAdjustment(KeyFrame* pKF, bool *pbStopFlag, Map *pMap);
  ```

  相比全局BA, 少了一些参数。

- 位姿优化：`int static PoseOptimization(Frame* pFrame);`
- 优化本质图：
  ```cpp
    void static OptimizeEssentialGraph(Map* pMap, 
        KeyFrame* pLoopKF, 
        KeyFrame* pCurKF,
        const LoopClosing::KeyFrameAndPose &NonCorrectedSim3,
        const LoopClosing::KeyFrameAndPose &CorrectedSim3,
        const map<KeyFrame *, set<KeyFrame *>> &LoopConnections,
        const bool &bFixScale);
  ```
  输入当前关键帧和找到的回环关键帧，做 Essential Graph 优化。 `bFixScale` 为真表示尺度需要优化（单目）。

- 优化 Sim3: 
  ```cpp
  static int OptimizeSim3(
    KeyFrame* pKF1, 
    KeyFrame* pKF2, 
    std::vector<MapPoint *> &vpMatches1,
    g2o::Sim3 &g2oS12, 
    const float th2, 
    const bool bFixScale);
  ```