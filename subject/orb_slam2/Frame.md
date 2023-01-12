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
