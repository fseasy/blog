---
layout: post
title: LocalMapping - ORB-SLAM2
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

局部地图模块。

## 主要成员

- 构造函数：`LocalMapping(Map* pMap, const float bMonocular);`
  接受地图、是否是单目。

- 线程主函数：`void Run();`

- 插入关键帧：`void InsertKeyFrame(KeyFrame* pKF);` 

- 线程同步操作：
  - 申请 reset
  - 申请 stop；检测是否 stop；检测是否已经接受到 stop 请求；设置不 stop
  - 设置是否接受新关键帧；检查当前是否接受关键帧；获取队列中关键帧数量
  - 发布
  - 打断 BA
  - 申请结束；检查是否已经结束

## 关键数据

- 新关键帧队列：类型为 `std::list<KeyFrame*>`
- 地图