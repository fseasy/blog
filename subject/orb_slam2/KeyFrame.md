---
layout: post
title: KeyFrame - ORB-SLAM2
date: 2023-01-13
categories: 技术
tags: vslam orb-slam2 code-reading
---

关键帧。非常重要的数据结构，主要参与到如下逻辑中：

- 位姿设置与获取
- 共视图构建
- 生成树
- 回环边关联
- 地图点关联

主要接口如下：

- `KeyFrame(Frame &F, Map* pMap, KeyFrameDatabase* pKFDB)`; 构造函数，接收 Frame, Map 和 KeyFrameDatabase. Frame 自然是 KeyFrame 的关键成员，而 Map 指针是用于处理地图点的， 关键帧数据库用于回环。

- 位姿处理： Set, Get(位置、位姿逆、相机光心、双目中心、旋转、平移).

- `void ComputeBoW();` 

- 共视图操作： 
  - `void AddConnection(KeyFrame* pKF, const int &weight)`; 添加边，看到边上有权重
  - `void EraseConnection(KeyFrame* pKF)`; 删除边，给定的是指针
  - 更新: `void UpdateConnections()`; `void UpdateBestCovisibles()`；
  - 获取关键帧，包括获取连接的关键帧集合、列表、Best-N 关键帧、权重 w 范围的关键帧
  - 获取边的权重： `int GetWeight(KeyFrame* pKF)`; 权重为整型

- 生成树操作: 添加、删除、获取、判断是否有子节点；改变、获取父节点

- 回环边操作：添加回环边、获取有回环边的关键帧集合

- 地图点操作：添加、删除、替换、获取地图点（观测）

- 关键点操作：`GetFeaturesInArea`, `UnprojectStereo`, 对 Frame 操作的代理

- 是否 Bad 的操作：是否运行设置 Bad；设置、判断 Bad.
