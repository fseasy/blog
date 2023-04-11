---
layout: post
title: LoopClosing - ORB-SLAM2
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

回环检测模块，需要插入帧到队列，然后在线程中运行主流程。

## 关键接口

- 构造函数：接受地图、关键帧数据库和词典。

  ```cpp
  LoopClosing(Map* pMap, KeyFrameDatabase* pDB, ORBVocabulary* pVoc,const bool bFixScale);
  ```

  `FixScale` 表示尺度是否固定。对单目而言，尺度是不固定的(所以是 7dof). 对双目/RGB-D, 尺度固定。

- 线程主函数：`void Run();`
- 插入新关键帧：`void InsertKeyFrame(KeyFrame *pKF);`
- 运行 Global BA (GBA)： `void RunGlobalBundleAdjustment(unsigned long nLoopKF);`
  还有查询 GBA 状态的函数： 是否 GBA 在运行、是否 GBA 已结束。
- 请求重置
- 请求结束、是否已结束。

## 关键数据

- 回环关键帧队列： `std::list<KeyFrame*>`
