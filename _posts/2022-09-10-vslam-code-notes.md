---
layout: post
title: VSLAM 代码笔记
date: 2022-09-10
categories: 技术 
tags: vslam code-notes
---
> 学习 vslam 过程中的一些代码记录。

## OpenCV 基础

1\. `cv::Mat` 用起来方便，但还是挺容易有坑的！最大的问题，就是 `.at<T>`, 如果 `T` 不是实际类型，运算结果会错误，但程序多半不会挂…… 譬如， `cv::recoverPose` api 返回的 `R`, 类型是 `CV_64F`, 即 `double`, 如果用 `.at<float>`, 结果就错了。这要求我们必须得知道输出的类型具体是啥。然而，api 上不一定会标注出来…… 

目前明确的类型：多视图几何相关的 api 里：相机矩阵K，一般要声明为`double`; R、t 都是 `double` 的；传给 `cv::triangulatePoints` 必须传入 `float`. 

尽量少用 `.at`, 可以使用 `.copyTo` 来赋值，这类函数，内部一般会做好类型转换，不会出问题。

## 相机与图像

1\. 像素点坐标 $p$ 到归一化平面上的3d坐标 $P$

按 $p = K P$ 看， $P = K^{-1} p $, 但没必要这样算，直接按照求 p 的逆运算即可：

- 先减去中心偏移
- 再除以缩放系数(focal)

```cpp
cv::Mat pixel_pnt2camera_3d(const cv::Point2f& p, const cv::Mat& camera_intrinsic) {
    auto K_ = static_cast<cv::Mat_<double>>(camera_intrinsic);
    double cx = K_(0, 2);
    double cy = K_(1, 2);
    double fx = K_(0, 0);
    double fy = K_(1, 1);
    return (cv::Mat_<double>(3, 1) << 
        (p.x - cx) / fx,
        (p.y - cy) / fy,
        1
    );
};
```

> 注1：经过实际测试，上述做法，和 $K^{-1} p$ 结果一致。

> 注2：代码中 `static_cast<cv::Mat_<double>>(camera_intrinsic)`，其实是调用了 `cv::Mat_` 的构造函数： `Mat_ (const Mat &m)`, 这个包含了拷贝或转换逻辑，是有代价的！尽量不应该这么做。

2\. 三角化计算

调用 OpenCV 的 `triangulatePoints`, 需要做如下步骤的准备：

1. 获得用于三角化的、来自两个图片的匹配点对 $\{(p1, q1), \cdots \}$
2. 根据相机内参 $K$, 将点对（即像素坐标）转为相机空间的归一化坐标 $\{(pn1, qn1), \cdots\}$. 特别注意，本来归一化坐标是3维的（最后 $z=1$），然而**这里只取 2 维**，即抛弃到 $z$！
3. 准备2个映射矩阵 $T$，维度是 $3 \times 4$. $T1$ 对应第一张图片，即基准图片， 则设置其为 $\begin{bmatrix} E & \boldsymbol{0} \end{bmatrix}$; $T2$ 对应第二张图片，即运动后的图片，设置其为 $\begin{bmatrix} R & \boldsymbol{t} \end{bmatrix}$. 可以通过 `.copyTo` 来赋值。
4. 保证点对、$T$ 都是 `float` 类型的！ 调用 `cv::triangulatePoints`, 得到的是一个 $4 \times N$ 的矩阵！
5. 对 $4 \times N$ 转换为 3 维的点列表； $p4 \rightarrow p3$ 时，每个维度除以第 4 维，再丢掉第 4 维就是转换后的结果。

关键代码如下：

```cpp
// pixel -> camera, and drop last dim
std::vector<std::vector<cv::Point2f>> match_points_camera(2);
for (std::size_t i = 0U; i < match_points.at(0).size(); ++i) {
    auto point1 = _pixel2camera(match_points.at(0).at(i));
    auto point2 = _pixel2camera(match_points.at(1).at(i));
    match_points_camera.at(0).push_back(cv::Point2f(point1.x, point1.y));
    match_points_camera.at(1).push_back(cv::Point2f(point2.x, point2.y));
}
// build T1, T2
cv::Mat T1 = (cv::Mat_<float>(3, 4) << 
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0
            );
cv::Mat T2(3, 4, CV_32F);
R.copyTo(T2.colRange(0, 3));
t.copyTo(T2.colRange(3, 4));
// triangulate and post-process
cv::Mat points4f{};
cv::triangulatePoints(T1, T2, 
    match_points_camera.at(0), match_points_camera.at(1),
    points4f);
// points4f shape = 4 x N
points3.clear();
for (int i = 0; i < points4f.cols; ++i) {
    auto p = points4f.col(i);
    p /= p.at<float>(3, 0);
    auto p_ = static_cast<cv::Mat_<float>>(p);
    points3.emplace_back(p_(0, 0), p_(1, 0), p_(2, 0));
}
```

> 注：为何三角化后是 4 维？是因为求解点时，需要用齐次坐标（[why](https://www.cnblogs.com/csyisong/archive/2008/12/09/1351372.html)）. 齐次坐标就是在尾部扩展 1 维，从而 3 维变 4 维； 最后处理第 4 维，其实就是齐次坐标变为笛卡尔坐标的处理[^1]。

> 三角化是通过直接线性变换法(DLT)实现的。具体原理和 plain 实现，参考 [三角化（码—opencv)](https://blog.csdn.net/AAAA202012/article/details/117396962). 

3\. PnP 中 3d 坐标准备

PnP 建模的是 2d 坐标和另一个坐标系下的 3d 点的关系。在高博的 PnP 示例里，实际拥有的数据如下： 

1. 基准图片关键点坐标；移动后的图片关键点坐标（与基准图片关键点一一对应）
2. 基准图片里各个像素的深度值

并没有直接的 3d 坐标，需要基于图片中像素坐标和深度值来计算 3d 坐标。因为深度值是相对与相机的，所以这个 3d 点坐标是相机坐标系下的。所以，计算方式为

1. 先将基准图片的像素坐标 $p$ 变换为相机坐标 $P_{norm}$（即用上面的`pixel_pnt2camera_3d`），此坐标落在相机坐标系里的归一化平面上（即 $Z = 1$）
2. 再将深度值 $d$ 乘上归一化坐标 $P_{norm}$, 即得到用于 PnP 计算的 3d 坐标了

```cpp
// 这里的片段展示了加载深度、构造2d、3d点对的过程。
// 一定要注意二者需要一一匹配：深度值可能会错误，这时匹配的 2d 点也要抛弃
using p3d2d_t = std::pair<std::vector<cv::Point3f>, std::vector<cv::Point2f>>;
p3d2d_t load_depth_and_make_3d2d_points(const std::string& depth_fpath,
    const std::vector<cv::Point2f>& points2d,
    const cv::Mat& K,
    const std::vector<cv::Point2f>& other_points2d) {
    auto depth_mat = cv::imread(depth_fpath, cv::IMREAD_UNCHANGED);
    std::vector<cv::Point3f> objects{}; // ignore .reserve for compact
    std::vector<cv::Point2f> img_points{};
    for (std::size_t i = 0U; i < points2d.size(); ++i) {
        auto& p = points2d.at(i);
        auto raw_depth = depth_mat.at<ushort>(p.y, p.x);
        // bad
        if (raw_depth == 0) { continue; }
        // as slambook example. I don't know why.
        float actual_depth = raw_depth / 5000.f;
        // 深度，其实是相机空间下的；所以，x，y也需要转换到相机空间下！
        cv::Point3f camera3d_norm = pixel_pnt2camera_3d(p, K);
        cv::Point3f camera3d = camera3d_norm * actual_depth;
        objects.push_back(std::move(camera3d));
        img_points.push_back(other_points2d.at(i));
    }
    return std::make_pair(std::move(objects), std::move(img_points));
}
```

[^1]: https://www.zhihu.com/question/59595799/answer/301242100 "什么是齐次坐标系?为什么要用齐次坐标系？ - 格东西的回答 - 知乎"

