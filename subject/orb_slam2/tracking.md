---
layout: post
title: Tracking - ORB-SLAM2
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

Tracking 主要职责为:

1. 给定帧，确定此刻相机的位姿
2. 确定何时插入新关键帧、创建一些地图点
3. 跟丢了重定位

`Tracking` 类定义在 `ORB_SLAM2` 命名空间下。

关键函数为：

成员变量为：

