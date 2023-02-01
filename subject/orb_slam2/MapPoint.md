---
layout: post
title: MapPoint - ORB-SLAM2
date: 2023-02-01
categories: 技术
tags: vslam orb-slam2 code-reading
---

世界坐标系下的地图点(3D)，是建图的输出，也深入参与定位过程。有如下关键字段：

- 3D 位置 WordPos, 这个是在世界坐标系下的坐标
- 法向量 NormalVecgtor, 这是所有视图向量的 mean unit vector.
- ORB 描述子 Descriptor, 是观测到这个Map Point的关键帧中，汉明距离最小的点对应的描述子 （啥意思还不懂）
- 最大d-max 和 最小 d-min, 这个是根据 ORB 特征尺度不变性的计算的点可被观测到的极限值

内部存储了观测到该地图点的关键帧及在帧内的 id (`std::map<KeyFrame*,size_t>` 类型).

### 主要接口

- 构造函数：
  - `MapPoint(const cv::Mat &Pos, KeyFrame* pRefKF, Map* pMap);`
  - `MapPoint(const cv::Mat &Pos,  Map* pMap, Frame* pFrame, const int &idxF);`
  
  两种构造函数不一样。第一个有一个 Reference KeyFrame 的参数，TODO
  第二个有帧和 帧内 id 的参数（注意，是帧，而非关键帧）。

  都有 `Pos` 坐标。

- 世界坐标相关： Get/Set
- 法向量相关：
  - `cv::Mat GetNormal();`
  - `void UpdateNormalAndDepth();`

- 描述子相关： 
  - `void ComputeDistinctiveDescriptors();`
  - `cv::Mat GetDescriptor();`

- 观测（能看到此 Map Point 的关键帧）操作
  - 添加、删除观测：`void AddObservation(KeyFrame* pKF,size_t idx);`, `void EraseObservation(KeyFrame* pKF);`
  - 获取观测： `std::map<KeyFrame*,size_t> GetObservations();`, `int Observations();`

- 在可视条件下找到的比例： +Visible, +Found, 计算比例，获取 Found 次数
- 是否在关键帧中， 在关键帧中的比例

- 替换、获取替换： `void Replace(MapPoint* pMP); `, `MapPoint* GetReplaced();`
- Bad 操作： 设置/获取 Bad 状态。

### 关键成员变量

- Map id
- Tracking 中记录状态信息的一系列变量
- Local Mapping 中记录状态信息的系列变量
- Loop Closing 中记录状态信息的系列变量

### 类静态成员

- nNextId: 用户获取全局唯一 Frame id

