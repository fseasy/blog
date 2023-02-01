---
layout: post
title: ORB-SLAM2 中的关键概念
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

KeyPoint 关键点：

MapPoint 地图点：同时定位与建图，所谓建图，就是这些计算得到这些地图点。

KeyFrame 关键帧：

Frame 帧:

covisibility-graph 共视图：

motion-model 匀速模型：

## 代码的缺点

1. 不同传感器耦合到 1 个类里，导致靠 flag、if 来区分逻辑，分支有点多
2. 构造函数参数太多，是否有些东西可以组合到一起，有的东西(map, vocab)可以用单例？
3. 【存疑】OOP 和 function 没有区分开，自己的成员函数当做 staitc-function 来写（就是要传入同类型别的实例）