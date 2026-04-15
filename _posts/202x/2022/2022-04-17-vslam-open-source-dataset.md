---
layout: post
title: 视觉 SLAM 著名开源项目及数据集
date: 2022-04-17
categories: 技术 
tags: SLAM
---
> 记录 V-SLAM 著名开源项目。

## 开源项目

### ORB-SLAM

地址：<https://webdiis.unizar.es/~raulmur/orbslam/>

适用于单目、双目、RGB-D相机；

> ORB-SLAM is a versatile and accurate SLAM solution for Monocular, Stereo and RGB-D cameras. It is able to compute in real-time the camera trajectory and a sparse 3D reconstruction of the scene in a wide variety of environments, ranging from small hand-held sequences of a desk to a car driven around several city blocks.

### PTAM

地址： <https://github.com/Oxford-PTAM/PTAM-GPL>

Parallel Tracking And Mapping, 并行跟踪和建图。一个线程做前端，一个线程做后端，这样后端的运行速度可以与视频速度解耦。

> PTAM is a monocular SLAM system useful for real-time 6-DOF camera tracking in small scenes. I

## 数据集

### BAL

全称： Bundle Adjustment in the Large

官网地址：<https://grail.cs.washington.edu/projects/bal/>

> Recent work in Structure from Motion has demonstrated the possibility of reconstructing geometry from large-scale community photo collections. Bundle adjustment, the joint non-linear refinement of camera and point parameters, is a key component of most SfM systems, and one which can consume a significant amount of time for large problems. 

看起来发布的时候是为了 SfM 任务的。发布的论文已经是 2010 年的了。

### 其他

Meshlab, 查看点云。

FAB-MAP([openFABMAP](https://github.com/arrenglover/openfabmap)), a Simultaneous Localisation and Mapping algorithm which operates solely in appearance space. 