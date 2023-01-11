---
layout: post
title: System - ORB-SLAM2
date: 2023-01-11
categories: 技术
tags: SLAM vslam orb-slam2
---
> System 是整个系统的接口类，暴露出接口与外部交互。

System.h 定义了类 `System`, 定义在 `ORB_SLAM2` 命名空间里面，

包含内部枚举： `eSensor`, 传感器类型。取值为 `MONOCULAR, STERO, RGBD`, 是传统的 `enum` 而非 `enum class`

关键函数如下：
- 构造函数：System(const string &strVoScFile, const string &strSettingsFile, const eSensor sensor, const bool bUseViewer = true);
  
  接收参数：
  
  - 词表文件：BOW的词表
  - 配置文件：一个 YAML 文件，里面包含了 3 大类参数：相机信息（内参、FPS、像素存储格式）、ORB抽取器（特征点数、金字塔设置、FAST 阈值设置）、Viewer 设置。
  - 传感器类型。
  - 是否使用 Viewer

- Track 函数： 按传感器类型有相应的 Track 函数：

  - cv::Mat TrackStereo(const cv::Mat &imLeft, const cv::Mat &imRight, const double &timestamp); 输入左、右两个视图的图片，再加时间戳。