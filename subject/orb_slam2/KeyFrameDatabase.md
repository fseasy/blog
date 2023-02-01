---
layout: post
title: KeyFrameDatabase - ORB-SLAM2
date: 2023-02-01
categories: 技术
tags: vslam orb-slam2 code-reading
---

关键帧数据库，包含对关键帧的添加删除和查找相似的操作。

### 关键接口

- 构造函数：`KeyFrameDatabase(const ORBVocabulary &voc);`
  接受 ORBVocabulary.

- 关键帧添加、删除：
  - `void add(KeyFrame* pKF);`
  - `void erase(KeyFrame* pKF);`

- 查找候选关键帧：
  - `std::vector<KeyFrame *> DetectLoopCandidates(KeyFrame* pKF, float minScore);` 回环帧候选，输入是关键帧。
  - `std::vector<KeyFrame*> DetectRelocalizationCandidates(Frame* F);` 重定位帧候选，输入是普通帧。

- 清理：`void clear();`

### 关键数据

- 倒排文件：`std::vector<list<KeyFrame*>>`