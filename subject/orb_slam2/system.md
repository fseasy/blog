---
layout: post
title: System - ORB-SLAM2
date: 2023-01-11
categories: 技术
tags: vslam orb-slam2 code-reading
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

- Track 函数：按传感器类型有相应的 Track 函数：

  - cv::Mat TrackStereo(const cv::Mat &imLeft, const cv::Mat &imRight, const double &timestamp); 输入左、右两个视图的图片，再加时间戳。左右图片需要同步，需要预先校正？
  - cv::Mat TrackRGBD(const cv::Mat &im, const cv::Mat &depthmap, const double &timestamp); 输入图片、深度图和时间戳。
  - cv::Mat TrackMonocular(const cv::Mat &im, const double &timestamp); 输入单目图片和时间戳。

  图片要么是 RGB/RBG(看设置) `CV_8UC3`，要么是灰度 `CV_8U`；内部都会转为灰度。看代码注释，双目需要预先校正，但是其他设置似乎不需要。

- 开关仅定位模式：
  - void ActivateLocalizationMode(); 
  - void DeactivateLocalizationMode();

- 检查地图是否大变：bool MapChanged(); 闭环检测、全局 BA 完成后会认为地图大变。

- 重置：void Reset(); 会清理掉地图

- 关闭所有进程：void Shutdown(); 等待直到所有进程关闭再返回

- 保存轨迹：

  - void SaveTrajectoryTUM(const string &filename); 只支持双目、RGBD
  - void SaveKeyFrameTrajectoryTUM(const string &filename); 支持所有传感器类型
  - void SaveTrajectoryKITTI(const string &filename); 不支持单目

- 状态获取

  - int GetTrackingState(); 当前的跟踪状态（返回 int 感觉不太好啊）
  - std::vector<MapPoint*> GetTrackedMapPoints();  地图点
  - std::vector<cv::KeyPoint> GetTrackedKeyPointsUn(); 关键点

成员变量如下：

- 核心数据： ORB 词典、关键帧 DB、地图、跟踪结果（地图点、关键点）
- 关键执行器： Tracker， LocalMapper, LoopCloser, Viewer+Drawer. 
- 执行器对应的线程句柄： LocalMapping, LoopClosing, Viewer. Tracking 就是 Sytem 的主线程（即 Track 函数)，故无单独的线程
- 其他设置项： 传感器类型、是否是仅定位、是否要重置.

