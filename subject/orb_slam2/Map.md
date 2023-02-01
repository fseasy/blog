---
layout: post
title: Map - ORB-SLAM2
date: 2023-02-01
categories: 技术
tags: vslam orb-slam2 code-reading
---

地图类，存储 MapPoints 和 KeyFrames. 

### 关键接口

- 构造函数： `Map();` 不带任何参数
- KeyFrame 操作： 
  - `void AddKeyFrame(KeyFrame* pKF);`
  - `void EraseKeyFrame(KeyFrame* pKF);`
- MapPoint 操作：
  - `void AddMapPoint(MapPoint* pMP);`
  - `void EraseMapPoint(MapPoint* pMP);`

- 设置 Reference Map Points: `void SetReferenceMapPoints(const std::vector<MapPoint*> &vpMPs);`

- 大更新：`void InformNewBigChange();` 和 `int GetLastBigChangeIdx();`.
- size： 地图的 Map Points 数、KeyFrame 数
- 清理： `void clear();`

### 关键数据

- Map Points 集合 `std::set<MapPoint*>`. 是把指针放到 set 里.
- KeyFrames 集合 `std::set<KeyFrame*>`, 数组 `vector<KeyFrame*>` (public 类型)。

