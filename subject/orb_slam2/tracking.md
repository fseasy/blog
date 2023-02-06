---
layout: post
title: Tracking - ORB-SLAM2
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

Tracking 主要职责为:

1. 初始化起始关键帧和地图点。
2. 给定帧，确定此刻相机的位姿
3. 确定何时插入新关键帧、创建一些地图点
4. 跟丢了重定位

## 关键接口

- 构造函数：
  ```c++
    Tracking(
        System* pSys, 
        ORBVocabulary* pVoc, 
        FrameDrawer* pFrameDrawer, 
        MapDrawer* pMapDrawer, 
        Map* pMap,
        KeyFrameDatabase* pKFDB, 
        const string &strSettingPath, 
        const int sensor);
  ```

## 逻辑流程

### 1. 构造函数

1. 使用初始化列表初始化成员变量，进入函数体
2. 从配置文件中读取相机参数并赋值： `K`(fx, fy, cx, cy), `DistCoef`(畸变参数), `bf`(基线长度), `fps`, `RGB`.
  - 通过 `cv::FileStorage` 加载配置，这是一个读写 XML/JSON/YAML 的[类](https://docs.opencv.org/4.x/da/d56/classcv_1_1FileStorage.html#a973e41cb75ef6230412a567723b7482d)。
  - fps 读取为 0，则设为默认值 30.
  - `RGB` 如果为假，则默认为 `BGR`. 灰度图则忽略此参数。

3. 