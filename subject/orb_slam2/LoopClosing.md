---
layout: post
title: LoopClosing - ORB-SLAM2
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

回环检测模块。

## 关键接口

- 构造函数：接受地图、关键帧数据库和词典。

  ```c++
  LoopClosing(Map* pMap, KeyFrameDatabase* pDB, ORBVocabulary* pVoc,const bool bFixScale);
  ```

- 线程主函数：`void Run();`
- 插入新关键帧：`void InsertKeyFrame(KeyFrame *pKF);`