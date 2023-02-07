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

```c++
Tracking(System *pSys, 
  ORBVocabulary* pVoc, 
  FrameDrawer *pFrameDrawer, 
  MapDrawer *pMapDrawer, 
  Map *pMap, 
  KeyFrameDatabase* pKFDB, 
  const string &strSettingPath, 
  const int sensor)
```

1. 使用初始化列表初始化成员变量，进入函数体
2. 从配置文件中读取相机参数并赋值： `K`(fx, fy, cx, cy), `DistCoef`(畸变参数), `bf`(按注释，等于基线长度乘上fx, 后面会用到这个定义), `fps`, `RGB`.
  - 通过 `cv::FileStorage` 加载配置，这是一个读写 XML/JSON/YAML 的[类](https://docs.opencv.org/4.x/da/d56/classcv_1_1FileStorage.html#a973e41cb75ef6230412a567723b7482d)。
  - fps 读取为 0，则设为默认值 30.
  - `RGB` 如果为假，则默认为 `BGR`. 灰度图则忽略此参数。

3. 从配置中读取 ORBExtractor 参数并初始化特征抽取器：
  - Monocular: 初始化 left 和 initializer 两个抽取器。left 就是普通的抽取器， initializer 是初始化时用的抽取器。
  - Stereo: 初始化 left 和 right 两个抽取器。
  - RGB-D: 只初始话一个 left.

4. 对 Stereo 和 RGB-D 初始化深度相关参数： 
  - Close/Far point 阈值点深度值。论文上说的是 `40 * baseline`, 代码里是 `dep_setting * bf / fx` , `dep_setting` 就是论文上的 40, 而 `bf / fx` 就是 baseline（上面的定义）
  - RGB-D，获取深度读数的缩放值：`DepthMapFactor`. 代码里先求倒数了，应该是后面避免除法运算来加速。（指令开销从小到大是：浮点乘 < 浮点除<sup>[1][1]</sup>）

### 2. 单目 Track 流程

```c++
cv::Mat GrabImageMonocular(const cv::Mat &im, const double &timestamp);
void Tracking::Track();
```
#### 2.1 GrabImageMonocular

先看 `GrabImageMonocular` 流程，是单目 Track 主流程，抽象得很简单：

1. 将输入 `im` 做必要的转换，存储到成员变量 `mIMGray`，。
  - 调用 [cvtColor](https://docs.opencv.org/3.4/d8/d01/group__imgproc__color__conversions.html#ga397ae87e1288a81d2363b61574eb8cab) 函数，三通道用 `cv::COLOR_RGB2GRAY`, 四通道用 `cv::COLOR_RGBA2GRAY`.

2. 根据当前跟踪状态，构造一个 `Frame` 并赋值给 `mCurrentFrame`. 
  - 如果是 `NOT_INITIALIZED` 或 `NO_IMAGES_YET`, 则用 initializer ORBExtractor 去初始化 Frame
  - 否则，就用 left ORBExtractor 初始化 Frame
3. 调用 `Track`
4. 返回 `mCurrentFrame.mTcw.clone()`

#### 2.2 Track

`Track` 函数是单目、双目等公共调用的函数，这里只看单目的逻辑：

1. 更新内部状态 `mState`：如果是当前状态是 `NO_IMAGES_YET`, 则变为 `NOT_INITIALIZED`. 其他不变。
2. 如果当前状态是 `NOT_INITIALIZED`, 则走 **单目初始化** `MonocularInitialization` 分支；
3. 否则，系统已经初始化。
  - 如果系统为*建图+定位*模式：
    - 如果内部状态为 `OK`, 则
      - 首先检查回环检测模块对地图点的更新：`CheckReplacedInLastFrame` (与回环同步)
      - 然后开始 Tracking. 
      - 如果匀速模型没有建立，或者当前帧距离上次重定位帧少于2帧（即是重定位之后的1帧），则用 `TrackReferenceKeyFrame` 来跟踪。
      - 否则，匀速模型是好的，帧也稳定，用 `TrackWithMotionModel` 来跟踪。如果跟踪失败，还是回退到 `TrackReferenceKeyFrame`.
    - 否则，当前跟踪不成功，调用 `Relocalization` 重定位。
  - 否则，当前是*仅定位*模式：
    - 



[1]: https://www.zhihu.com/question/458395216/answer/1876737852 "C语言 乘以0.01快？还是除以100快？ - 北极的回答 - 知乎"