---
layout: post
title: Frame - ORB-SLAM2
date: 2023-01-12
categories: 技术
tags: vslam orb-slam2 code-reading
---

Frame 接收原始输入信息（图像+深度），并具备如下功能
1. 金字塔特征抽取（ORB）：得到关键点（KP）、关键点描述
2. 匹配左右视图的 KP 并计算深度
3. 计算、存储整个帧的 BoW
4. 存储位姿（似乎本身不计算位姿）

关键点 KP 使用的就是 `cv::KeyPoint`, 它普遍用来存储特征抽取后的关键点信息，主要包含位置、方向、金字塔层次等信息。

关键接口如下：

- 构造函数: `Frame(img[+depth], const double &timeStamp, ORBextractor* left[+right], ORBVocabulary* voc, cv::Mat &K, cv::Mat &distCoef, const float &bf, const float &thDepth)`;
  
  可以看到，输入首先是图片（+深度）、时间戳，这个也是 Track 的输入；然后是 ORB 特征抽取器，说明特征抽取要在这里做；再是词典，要算帧的向量表示； K、distCoef、bf（相机 baseline) 是相机信息, 处理图片需要的；thDepth 是判定远、近点的阈值。

- `void ExtractORB(int flag, const cv::Mat &im)`; 抽 ORB 特征， flag 指示左/右图片；奇怪为啥 im 又要传进来？构造函数不是传了吗

- `void ComputeBoW()`; 计算帧的 BoW 表示

- 位姿相关
  - `void SetPose(cv::Mat Tcw)`; 设置位姿
  - `void UpdatePoseMatrices()`; 基于位姿更新内部位姿相关的变量（R, t, 相机中心等额外矩阵）
  - `cv::Mat UnprojectStereo(const int &i)`; 把 1 个 KP 反向映射到世界坐标
  - `void ComputeStereoMatches()`; `void ComputeStereoFromRGBD(const cv::Mat &imDepth)`; 左边的 KP 在右边关联上KP，关联上的话，存储/计算关联的右边KP和深度值

- 区域判定：
  - `bool isInFrustum(MapPoint* pMP, float viewingCosLimit)`; 判断这个地图点是否在相机的视锥(view frustum)中。
  - `bool PosInGrid(const cv::KeyPoint &kp, int &posX, int &posY)`; KP 是否在格子里

- 获取类：相机中心、Rotation-Inverse、Features-in-Area
