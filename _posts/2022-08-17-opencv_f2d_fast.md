---
layout: post
title: OpenCV FAST 角点检测算法 CPU 版本实现源码注解
date: 2022-08-17
categories: 技术 
tags: OpenCV FAST角点检测 源码注解
---
> 看高博的“视觉里程计”章节，以为 FAST 算法很简单， 但最近在 OpenCV 的 tutorial 里，发现 FAST 角点
检测算法没那么 Naive，还有 Machine Learning 在里面呢！ 

<!-- code is too long, enlarge the page -->
<style>
    .container {
        width: 1200px;
    }
</style>

从 [FAST Algorithm for Corner Detection][fast_opencv_tutorial] 中看到，该算法远比高博书里讲得复杂。其甚至包含一个 ID3 的决策数来确定怎么选点来让整个检测更快速， 而且注意到其论文名字就是 *Machine learning for high-speed corner detection*. 那问题来了，既然有 ML，那肯定得有模型吧，
OpenCV 里是咋做的呢？ 是预先训练好的，开发者直接用？ 还是有一个 train 接口，支持在目标数据上重训呢？
另外，我之前一直以为 FAST-12，就是只要有至少 12 个点和中心点有亮度差异就行了，但是看这个 tutoral/论文，
看起来要求：

1. 12点必须连续
2. 这些点的亮度差异类型得一样（要么都是darker, 要么都是 brighter).

而且，之前以为 FAST 算法的参数如高博所言，是 FAST-N, 但是看起来参数是亮度的阈值啊？ 谁说得对？
网上查，中英文都搜了，是真的找不到有效信息。这 OpenCV 的资源，好像不是那么丰富啊。只有看源代码了！

辗转确定了 FAST 算法的实现位置，在 [opencv/opencv:modules/features2d/src/fast.cpp][fast_code].
代码拉下来，费了几天劲注解了一下代码，这里贴上来。
需要注意的是，因为 OpenCV 要追求速度，所以在不同硬件、软件环境下有不同的实现，如 OpenGL, OpenVX, HAL 等实现，即使最基本的 CPU 实现，还有 SIMD 等宏分支。这里我能力、时间有限，只看了最基本的 CPU 实现。

## 代码注解

核心的就 2 个文件， `fast.cpp` 和 `fast_score.cpp`. 
前者是接口核心实现，后者定义了前者的2个辅助函数：计算周围像素偏移和计算角点分数。

### 1. fast.cpp

{% highlight cpp linedivs %}
// 一大段版权注释，这里不做删减。前后还是有一定信息量的
//
/* This is FAST corner detector, contributed to OpenCV by the author, Edward Rosten.
   Below is the original copyright and the references */

/*
Copyright (c) 2006, 2008 Edward Rosten
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    *Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

    *Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

    *Neither the name of the University of Cambridge nor the names of
     its contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
The references are:
 * Machine learning for high-speed corner detection,
   E. Rosten and T. Drummond, ECCV 2006
 * Faster and better: A machine learning approach to corner detection
   E. Rosten, R. Porter and T. Drummond, PAMI, 2009
*/

#include "precomp.hpp"
#include "fast.hpp"
#include "fast_score.hpp"
#include "opencl_kernels_features2d.hpp"
#include "hal_replacement.hpp"
#include "opencv2/core/hal/intrin.hpp"
#include "opencv2/core/utils/buffer_area.private.hpp"

#include "opencv2/core/openvx/ovx_defs.hpp"

namespace cv
{

/****************************************************************
 * FAST_t 是 CPU 基础版本的核心函数。后面的一些函数、类都是基于这个的包装
 ****************************************************************/
// 基本的 CPU 实现； 
// patternSize 就是看周围多少个点，目前根据 type 的不同，可取值 8, 12, 16； 下面都以常见的 16 为例子算
// InputArray = const _InputArray&, 
//              而 _inputArray 又是一个万金油的包装类，方便接受任何类型(Mat, Vector等)的参数, 可类比 c中void*, Java的Object 等
//              故这个就是表示一个不可变的图像输入。
template<int patternSize>
void FAST_t(InputArray _img, std::vector<KeyPoint>& keypoints, int threshold, bool nonmax_suppression)
{
    // 从包装类，先拿到实际的 Mat （类似与把一个 void* 变量 static_cast 到具体类型这种； 说明这个函数只处理 Mat 类输入） 
    Mat img = _img.getMat();
    // 以 t = 16 为例子, K = 8; N = 25
    // 从后文看到，K 就是最少符合要求的点数量的下限（不含）。
    // 如这里是 16-9 模式，即至少要有 9 个点满足灰度阈值限制，则这里 K = 8
    // N 则是为了从当前位置往后连续匹配方便，建立的缓冲区大小。如 16-9 模式下，对第16个点，还要往后看8个点，
    // 则为了避免取模操作，直接将数组填充为 16 + 8 + 1 = 25（填充时按取模的方式填充，达到循环的效果）
    // 但是为何要多加一个 `1`, 我也没想明白 
    const int K = patternSize/2, N = patternSize + K + 1;
    int i, j, k, pixel[25];
    // img.step 表示图像的一行需要的字节大小。
    // makeOffsets 实现在自 fast_score.cpp 中；
    // 签名为 void makeOffsets(int pixel[25], int rowStride, int patternSize)
    // 功能是提前算好周围元素相对与中心点的偏移量（底层表示的偏移，即一维、字节层面的偏移）。
    makeOffsets(pixel, (int)img.step, patternSize);

    keypoints.clear();
    // 保证输入的 threshold 有效； 255 说明这个函数处理 灰度的 8UC1 类型输入
    threshold = std::min(std::max(threshold, 0), 255);

    // 预先填充好灰度差值对应的标签结果
    // 像素差值 
    //    < -threshold, 为1； 
    //    > threshold, 为2； 
    //      否则为 0
    // 分开处理正负阈值，说明是区分低于中心亮度和高于中心亮度的。
    uchar threshold_tab[512];
    for( i = -255; i <= 255; i++ )
        threshold_tab[i+255] = (uchar)(i < -threshold ? 1 : i > threshold ? 2 : 0);

    // buf: （行）缓冲区； 由后可知，是放nonmax-suppression时要用的分数(uchar即可，因为分数在灰度区间内)
    //      这里申请了3行； 注意到type 是 uchar, 故是只能处理 8UC1 的图像（即灰度图）的！
    // cpbuf: （行）缓冲区， 从后面可知为 CornerPos buf 的缩写；存的是当前行里角点的 col 值；
    //        下面看到每行申请的 size 为 cols + 1, 
    //        比 cols(row-size) 多申请了 1 个， 后面可以看到，它是是来存有效角点个数的（即数组实际size）
    // BufferArea::allocate, 内存池化技术，相当于 new [xxx]
    // https://docs.opencv.org/4.4.0/d8/d2e/classcv_1_1utils_1_1BufferArea.html#details
    uchar* buf[3] = { 0 };
    int* cpbuf[3] = { 0 };
    utils::BufferArea area;
    for (unsigned idx = 0; idx < 3; ++idx)
    {
        area.allocate(buf[idx], img.cols);
        area.allocate(cpbuf[idx], img.cols + 1);
    }
    area.commit();

    for (unsigned idx = 0; idx < 3; ++idx)
    {
        memset(buf[idx], 0, img.cols);
    }

    // 逐行迭代！ 起始为 第 4 行！
    for(i = 3; i < img.rows-2; i++)
    {
        /*********************************************************************
         *  下面找此行中的角点
        ***********************************************************************/
        // img.ptr<T> 是 OpenCV 中迭代像素（差不多）最快的方式
        // 它按行迭代， .ptr<T>(i) 就是得到第 i 行元素的指针； 访问第 i 行的第j列，直接用C++原生的下标操作 [j] 即可
        // --- PS: 对(数组）指针a， a[j] 就是得到第j个元素的地址 addr = a + j * sizeof(*a)
        // .ptr(i) 与 .at(i) 都是访问第 i 行头地址，但是 .at 有边界检查，所以会慢一点……
        // 因此下面的 ptr， 就是图像 (i, 3) 位置的像素地址！ 
        // 结合 i 从 row = 3 开始，可知迭代都是从 第4行(0-based)、第4列 开始的！
        const uchar* ptr = img.ptr<uchar>(i) + 3;
        // curr：由后可知，存储是当前行个角点的分数（用于nonmax-suppression)，
        //         赋值为 buf 的第 x 个数组（长度为 cols = row size）； 
        //         (i - 3) %3 使得消除 i 从 3 开始的偏移，且保证取值在 {0，1，2} 中 ( 前面只申请了 3 个长度)
        uchar* curr = buf[(i - 3)%3];
        // cornerpos 由后可知，存当前行下，各角点的col坐标的； 
        //    注意， 从 cornerpos 可以推出 cpbuf 就是 CornerPos 的 buf, 即角点位置的缓冲区，所以类型是int
        //    如原生注释， cornerpos[-1] 要存值，所以 cornerpos 是从 cpbuf[k] 往后偏移1个位置，故 cpbuf 每行申请的空间是
        //    cols + 1 (row-size + 1)
        int* cornerpos = cpbuf[(i - 3)%3] + 1; // cornerpos[-1] is used to store a value
        // 迭代前重新清空像素缓冲
        memset(curr, 0, img.cols);
        int ncorners = 0;

        // 只在 倒数第4行 进入（比迭代终止条件少 1 行）
        if( i < img.rows - 3 )
        {
            /***************************************************************
             *  下面是判定当前点是角点的逻辑
             *  先通过位运算快筛，再验证是否满足：连续的、同一类型的亮度变化
            *****************************************************************/
            // 迭代： j 从第 4 列开始，倒数第 4 列结束；
            //       每次循环 j++，即递增一列，相应的ptr （指向原图像像素）也相应递增1， 与 j 对应
            // 每个像素，就是角点检测的中心像素
            j = 3;
            for( ; j < img.cols - 3; j++, ptr++ )
            {
                // 中心像素的灰度值
                // ptr 初始是 当前行第 4 列；这里将此位置的像素值取出赋给v； 
                //     这里有隐式类型转换（提升）: uchar -> int; 安全的
                int v = ptr[0];
                // 预先计算周围像素相对当前中心像素阈值的初始位置（效率优化）
                // 正常计算，应该是 threshold_tab[周围像素亮度 - v + 255]
                // = *(&threshold_tab[0] + 周围像素亮度 - v + 255)
                // 这里把 (&threshold_tab[0] - v + 255) 提出来，后面就只需 `+ 周围像素亮度 `
                // 就得到对应的阈值结果了！ => 挺巧妙的，但我肯定愿意写一个lamda函数来做…… 不过这肯定快
                // PS: &threshold_tab[0] 就等价与 threshold_tab 吧？都是数组开始的地址，前者太长了
                const uchar* tab = &threshold_tab[0] - v + 255;
                // 快速筛选： 测试 第0 和 第9 个元素（根据offset，即pixel定义，是最下面和最上面的像素）的阈值
                int d = tab[ptr[pixel[0]]] | tab[ptr[pixel[8]]];
                // < -T 是 01 (1), > T 是 10 (2), 无差异是 00 (0)
                // 上面是'或'，所以只要有1个有差异，下面就筛选就过了；
                if( d == 0 )
                    continue;
                // 连续检查 3 组；注意左边是 &=; 
                // 每对，至少一个点得有差异；整体上，差异还不能刚好相反 
                // —— 有点复杂，不知道究竟限制的是什么条件, 继续往后看
                d &= tab[ptr[pixel[2]]] | tab[ptr[pixel[10]]];
                d &= tab[ptr[pixel[4]]] | tab[ptr[pixel[12]]];
                d &= tab[ptr[pixel[6]]] | tab[ptr[pixel[14]]];

                if( d == 0 )
                    continue;
                // 继续检查 4 组；同样的限制
                d &= tab[ptr[pixel[1]]] | tab[ptr[pixel[9]]];
                d &= tab[ptr[pixel[3]]] | tab[ptr[pixel[11]]];
                d &= tab[ptr[pixel[5]]] | tab[ptr[pixel[13]]];
                d &= tab[ptr[pixel[7]]] | tab[ptr[pixel[15]]];
                // 到这里，因为共有 8 对点连续参与了  &= 运算，
                // 所以，如果有同样阈值的点小于8个，则 d 必然是 0；因为要想最后不是0，则每一对 &= 都得有1个bit位相同，即
                // 最少 8 个点的bit位相同。
                // 所以，要使 b 不为0，只有 3 种可能：
                // 1. >= 9 个是更暗的点 (< -T) => b = 01 (1)
                // 2. >= 9 个是更亮的点 (> T) => b = 10 (2)
                // 3. 8个更亮的点， 8 个更暗的点 => b = 11 (3)
                //
                // 下面需要再进一步区分 b 的情况，并且做更严格的判定： 
                //     **不仅要求个数，还得要求他们是连续的！！!**
                // 进入此 if 条件：d & 01 != 0;  则至少 8 个点的阈值是 1 （即 < -T), d 可能是01, 11
                if( d & 1 )
                {
                    // < -T 的情况（更暗）
                    // 这里就是要具体测试 周围点 - 中心点 < -T 的个数，即计算
                    //  count(x - v < -threshold)
                    // 与前面做 threshold_tab 加速一样，这里预先把 v 移过来，变为计算 count(x < v - threshold)
                    // 右边就是下面的 vt, 后面只需比较 x 与 vt 的值即可。
                    int vt = v - threshold, count = 0;
                    // 注意 k 的上限是 N, 在 t = 16 情况下， N = 25
                    for( k = 0; k < N; k++ )
                    {
                        // 这里就是取 周围像素的灰度值；
                        int x = ptr[pixel[k]];
                        if(x < vt)
                        {
                            if( ++count > K )
                            {
                            // 进入条件，找到了至少 9 个阈值 < -T 的点（更暗的点）；=> 当前像素是角点！
                                // cornerpos 存储当前角点的x值（col值）
                                // 循环内，都是在一个行下面，所以只需要存储col值即可。
                                cornerpos[ncorners++] = j;
                                // 如果需要做 nonmax-suppression (非极大值抑制)，就算当前中心点的分
                                //    这个分，必然是一个正数，且范围在 [treshold - 1, 255] 间，故 uchar 就可以承接
                                // cornerScore的函数签名为
                                //   int cornerScore<SZ>(const uchar* ptr, const int pixel[], int threshold)
                                // - ptr 在循环内，是中心点在矩阵的偏移； pixel 是周围点的相对偏移； 
                                //   threshold 即输入的灰度阈值
                                // - 返回的int被转为 uchar，作为角点的分数
                                // 注意注意，结果是存在 curr[j] 的，即角点所在的 col 位置；
                                // 因为不是每个 j 都是角点，所以 curr 大部分元素都没有填充这个分数 —— 默认值在前面，填充为0了！
                                // 后面做 nonmax-suppression 时会用到这个信息；
                                if(nonmax_suppression)
                                    curr[j] = (uchar)cornerScore<patternSize>(ptr, pixel, threshold);
                                break;
                            }
                        }
                        // 注意啊，注意！
                        // 这里一旦有1个点不满足 < -T，count 直接就重置为0了！！
                        // 这里可以说明2个问题：
                        // 1. 如论文所言， FAST是要求**连续的 >K个**像素的亮度差 < -T
                        // 2. N 比 K 大的由来： 因为t = 16，至少要连续9个点满足情况；
                        //    把将最后一个像素作为检查起点时(idx = 15)，还需要往后看 8 个, idx = 23；
                        //    则列表最长要 size = 24 …… 但，为何 N 是 25 呢？
                        //    感觉对位置0，冲突匹配了 1 次？ 反正这么写没问题，就是不知道是否有浪费。
                        else
                            count = 0;
                    }
                }

                // 进入此 if 条件：d & 10 != 0;  则至少 8 个点的阈值是 2 （即 > T), d 可能是10, 11
                if( d & 2 )
                {
                    // 这里就是 > T 的情况了（更亮）
                    // 整个逻辑同上，不赘述。
                    int vt = v + threshold, count = 0;

                    for( k = 0; k < N; k++ )
                    {
                        int x = ptr[pixel[k]];
                        if(x > vt)
                        {
                            if( ++count > K )
                            {
                                cornerpos[ncorners++] = j;
                                if(nonmax_suppression)
                                    curr[j] = (uchar)cornerScore<patternSize>(ptr, pixel, threshold);
                                break;
                            }
                        }
                        else
                            count = 0;
                    }
                }
                // 其他情况不必再测试，肯定不是角点了。
                // 注意到 b = 11 进入到了两个 if 判定中。这个没有问题，因为内部会更严格判定。
                // 11的情况，不是角点, 在内部会被过滤掉
            }
        }

        // 如前，cornerpos 初始时 = cpbuf + 1; 所以这里 -1， 其实对应到 cpbuf 的位置；
        // 存的是 该行的角点数。 很合理，毕竟要有变量存数组的大小！（这里靠数组值其实也能判定，只是不太通用）
        cornerpos[-1] = ncorners;

        // 要到3行中的中间行时，才处理上一行； i == 3 时，只有第一行被处理了，没有上一行，所以要跳过后面的处理；
        if( i == 3 )
            continue;

        /*******************************************************************************
        *     下面的部分，就是把最终的 KeyPoint 放进来，涉及到 nonmax-suppression             *
        ********************************************************************************/
        // 上一行就是 (i - 1) % 3, 上上行就是 (i - 2) % 3. 很直观， 不知道为啥下面要 -4 + 3 ……
        const uchar* prev = buf[(i - 4 + 3)%3];
        const uchar* pprev = buf[(i - 5 + 3)%3];
        // 这里看的是上一行的 corner pos & corner number; 但是变量又没有和上面一致，不太好！
        cornerpos = cpbuf[(i - 4 + 3)%3] + 1; // cornerpos[-1] is used to store a value
        ncorners = cornerpos[-1];

        for( k = 0; k < ncorners; k++ )
        {
            j = cornerpos[k];
            int score = prev[j];
            // 满足下面任一条件，就把上一行算出的这个角点放到最终的 KeyPoint 结果中
            // 1. 不开启 nonmax_suppressoin 【显然，直接】
            // 2. 当前像素的 score, 比
            //      同行左边1个、同行右边1个
            //      上边一行左边1个，上边一行对应位置，上边一行右边1个
            //      下边一行左边1个，下边一行对应位置，下边一行右边1个
            //    ==> 注意2个点： a. 如果比较的位置不是角点，则 score 默认都是0 
            //                  b. j + 1, j - 1 都不会越界！ 因为 j 从 3 开始，且小于 cols - 3
            if( !nonmax_suppression ||
               (score > prev[j+1] && score > prev[j-1] &&
                score > pprev[j-1] && score > pprev[j] && score > pprev[j+1] &&
                score > curr[j-1] && score > curr[j] && score > curr[j+1]) )
            {
                // KeyPoint 的定义:
                // 
                keypoints.push_back(KeyPoint((float)j, (float)(i-1), 7.f, -1, (float)score));
            }
        }
    }
}

// 最主要的入口函数。 下面的类、少参数的版本，核心都是这个函数。
// 这个函数，最基础的cpu实现，就是前面的 FAST_t 函数
void FAST(InputArray _img, std::vector<KeyPoint>& keypoints, int threshold, 
    bool nonmax_suppression, FastFeatureDetector::DetectorType type) {
    // 做性能测试的代码
    CV_INSTRUMENT_REGION();

    // 如果满足条件，跑 OpenCL 实现的版本并返回；
    CV_OCL_RUN(_img.isUMat() && type == FastFeatureDetector::TYPE_9_16,
               ocl_FAST(_img, keypoints, threshold, nonmax_suppression, 10000));

    // CALL_HAL 解释可见 https://www.cnblogs.com/willhua/p/12521581.html
    // 一样的，没有对应版本就走普通版本
    cv::Mat img = _img.getMat();
    CALL_HAL(fast_dense, hal_FAST, img, keypoints, threshold, nonmax_suppression, type);

    size_t keypoints_count;
    CALL_HAL(fast, cv_hal_FAST, img.data, img.step, img.cols, img.rows,
             (uchar*)(keypoints.data()), &keypoints_count, threshold, nonmax_suppression, type);
    // 一样的，尝试用 OpenVX 的实现
    CV_OVX_RUN(true,
               openvx_FAST(_img, keypoints, threshold, nonmax_suppression, type))

    // 最普通的实现了
    switch(type) {
    case FastFeatureDetector::TYPE_5_8:
        FAST_t<8>(_img, keypoints, threshold, nonmax_suppression);
        break;
    case FastFeatureDetector::TYPE_7_12:
        FAST_t<12>(_img, keypoints, threshold, nonmax_suppression);
        break;
    case FastFeatureDetector::TYPE_9_16:
        FAST_t<16>(_img, keypoints, threshold, nonmax_suppression);
        break;
    }
}

// 一个省略 FastFeatureDetector::DetectorType 的函数版本，应该是为了向后兼容？ 否则直接用默认形参即可？
void FAST(InputArray _img, std::vector<KeyPoint>& keypoints, int threshold, bool nonmax_suppression)
{
    CV_INSTRUMENT_REGION();

    FAST(_img, keypoints, threshold, nonmax_suppression, FastFeatureDetector::TYPE_9_16);
}


// 进一步封装为类的实现。这个是目前应该被使用的接口！(从类名看，不应该通过直接创建该类来使用，而是用下面的工厂函数)
// 里面的 detect 包含了图片类型自动转换（输入兼容性逻辑）、 调用 FAST、pixelMask 逻辑（猜测是基于给定 mask 矩阵去除一些角点）
// 不是核心逻辑，这里不再注释了
class FastFeatureDetector_Impl CV_FINAL : public FastFeatureDetector
{
public:
    FastFeatureDetector_Impl( int _threshold, bool _nonmaxSuppression, 
        FastFeatureDetector::DetectorType _type ) : 
            threshold(_threshold), 
            nonmaxSuppression(_nonmaxSuppression), type(_type)
    {}

    void detect( InputArray _image, std::vector<KeyPoint>& keypoints, InputArray _mask ) CV_OVERRIDE
    {
        CV_INSTRUMENT_REGION();

        if(_image.empty())
        {
            keypoints.clear();
            return;
        }

        Mat mask = _mask.getMat(), grayImage;
        UMat ugrayImage;
        _InputArray gray = _image;
        if( _image.type() != CV_8U )
        {
            _OutputArray ogray = _image.isUMat() ? _OutputArray(ugrayImage) : _OutputArray(grayImage);
            cvtColor( _image, ogray, COLOR_BGR2GRAY );
            gray = ogray;
        }
        FAST( gray, keypoints, threshold, nonmaxSuppression, type );
        KeyPointsFilter::runByPixelsMask( keypoints, mask );
    }

    void set(int prop, double value) { ... }
    double get(int prop) const {  }
    void setThreshold(int threshold_) CV_OVERRIDE { threshold = threshold_; }
    int getThreshold() const CV_OVERRIDE { return threshold; }
    void setNonmaxSuppression(bool f) CV_OVERRIDE { nonmaxSuppression = f; }
    bool getNonmaxSuppression() const CV_OVERRIDE { return nonmaxSuppression; }
    void setType(FastFeatureDetector::DetectorType type_) CV_OVERRIDE{ type = type_; }
    FastFeatureDetector::DetectorType getType() const CV_OVERRIDE{ return type; }

    int threshold;
    bool nonmaxSuppression;
    FastFeatureDetector::DetectorType type;
};

// 工厂函数，创建上面的实例指针
Ptr<FastFeatureDetector> FastFeatureDetector::create( int threshold, bool nonmaxSuppression, 
    FastFeatureDetector::DetectorType type ) {
    return makePtr<FastFeatureDetector_Impl>(threshold, nonmaxSuppression, type);
}

String FastFeatureDetector::getDefaultName() const {
    return (Feature2D::getDefaultName() + ".FastFeatureDetector");
}

} // end of namespace cv
{% endhighlight %}

### 2. fast_score.cpp

{% highlight cpp linedivs %}
#include "fast_score.hpp"
#include "opencv2/core/hal/intrin.hpp"
#define VERIFY_CORNERS 0

namespace cv {

// 预先计算周围点相对与中心点的偏移量（最底层的偏移量，即矩阵打平为一维、字节维度）
void makeOffsets(int pixel[25], int rowStride, int patternSize)
{
    // 半径为 3 的圆覆盖的像素范围 —— 相对中心点的 offset
    // 从后面来看，offset 的坐标格式是 (x, y). 因为图偏Y轴正半轴向下，因此
    // 编号顺序为：最下面的点开始，沿逆时针编号
    static const int offsets16[][2] =
    {
        {0,  3}, { 1,  3}, { 2,  2}, { 3,  1}, { 3, 0}, { 3, -1}, { 2, -2}, { 1, -3},
        {0, -3}, {-1, -3}, {-2, -2}, {-3, -1}, {-3, 0}, {-3,  1}, {-2,  2}, {-1,  3}
    };

    // 半径为 2 的圆；最下点、逆时针编号
    static const int offsets12[][2] =
    {
        {0,  2}, { 1,  2}, { 2,  1}, { 2, 0}, { 2, -1}, { 1, -2},
        {0, -2}, {-1, -2}, {-2, -1}, {-2, 0}, {-2,  1}, {-1,  2}
    };

    // 半径为 1 的圆； 最下点、逆时针编号
    static const int offsets8[][2] =
    {
        {0,  1}, { 1,  1}, { 1, 0}, { 1, -1},
        {0, -1}, {-1, -1}, {-1, 0}, {-1,  1}
    };

    // 只支持这3种，否则 offsets = nullptr, 下面的 CV_Assert 通不过
    const int (*offsets)[2] = patternSize == 16 ? offsets16 :
                              patternSize == 12 ? offsets12 :
                              patternSize == 8  ? offsets8  : 0;

    CV_Assert(pixel && offsets);

    // 预先计算好每个位置相对中心点的一维偏移 （y 轴offset * row-size + x轴 offset）
    int k = 0;
    for( ; k < patternSize; k++ )
        pixel[k] = offsets[k][0] + offsets[k][1] * rowStride;
    // 这里是超过 patternSize 的部分，重复之前的偏移
    for( ; k < 25; k++ )
        pixel[k] = pixel[k - patternSize];
}

// 16-9 模式下，计算用于 nonmax-suppression 的角点分数！
// 究竟算的啥值没太看懂，反正最终结果应该是一个 >= threshold - 1 的数， 且 <= 255
// 这块的具体逻辑，或许可以看这里：
// https://stackoverflow.com/questions/67306891/algorithm-behind-score-calculation-in-fast-corner-detector
template<>
int cornerScore<16>(const uchar* ptr, const int pixel[], int threshold)
{
    // 注意这里的 + 1
    const int K = 8, N = K*3 + 1;
    int k, v = ptr[0];
    // 计算的是中心点灰度与各个周围点的灰度值的差
    short d[N];
    for( k = 0; k < N; k++ )
        d[k] = (short)(v - ptr[pixel[k]]);
    {

        int a0 = threshold;
        // 下面要更新 a0; 
        // 更新逻辑是： a0 为 {每隔1个位置起，连续10个元素的的最小值} 构成的集合（共16个最小值） 中的最大值
        // a0 初始为 threshold, 而 threshold > 0 且 里面小于 a0 直接continue，故 最终的 a0 必然大于等于 threshold
        // 是一个正值！
        // 注意 K+=2； 且 看 10 个元素 （所以前面 N 要 K*3 + 1）
        for( k = 0; k < 16; k += 2 )
        {
            int a = std::min((int)d[k+1], (int)d[k+2]);
            a = std::min(a, (int)d[k+3]);
            if( a <= a0 )
                continue;
            a = std::min(a, (int)d[k+4]);
            a = std::min(a, (int)d[k+5]);
            a = std::min(a, (int)d[k+6]);
            a = std::min(a, (int)d[k+7]);
            a = std::min(a, (int)d[k+8]);
            a0 = std::max(a0, std::min(a, (int)d[k]));
            a0 = std::max(a0, std::min(a, (int)d[k+9]));
        }

        int b0 = -a0;
        // 下面更新 b0; 
        // 更新逻辑： 取 {每隔1个位置起，连续10个元素中最大值} 构成的集合中的 最小值；
        // b0 从 -a0 初始化，因为 a0 是>=threshold的，故 b0 初始是<=-threshold的一个负数
        // 最终也是一个 <= -threshold 的负数
        for( k = 0; k < 16; k += 2 )
        {
            int b = std::max((int)d[k+1], (int)d[k+2]);
            b = std::max(b, (int)d[k+3]);
            b = std::max(b, (int)d[k+4]);
            b = std::max(b, (int)d[k+5]);
            if( b >= b0 )
                continue;
            b = std::max(b, (int)d[k+6]);
            b = std::max(b, (int)d[k+7]);
            b = std::max(b, (int)d[k+8]);

            b0 = std::min(b0, std::max(b, (int)d[k]));
            b0 = std::min(b0, std::max(b, (int)d[k+9]));
        }

        // 最终， threshold 取 -b0 -1; -b0 是 >= 原 threshold 的正数
        threshold = -b0 - 1;
    }
    return threshold;
}

} // namespace cv
{% endhighlight %}

## 实现总结

1. OpenCV 的 FAST 算法实现，没有 Machine Learning 部分。
2. 但是角点判定的逻辑和原论文描述一致： 需要周围至少*连续的*、*同样亮度类型*的K个点，才是角点
3. OpenCV 的实现，FAST 只有 16-9, 12-7, 8-5 这 3 种类型，也就是圆半径分别为 3、2、1 的情况。在一种半径下，不能指定 K 值（即高博书中的 FAST-N 的 N)
4. 周围点的编号，与论文的图不一致。论文正上方编号为1，然后按顺时针编号；OpenCV代码里是最下方的点编号为1，逆时针编号。
5. 非极大值抑制，对比的是半径为1的点（共8个点）的像素分数（如果是角点就是对应角点计算的分数，否则就是0），比所有大才是 KeyPoint. 
6. 角点分数，不是取中心点与所有周围点像素差异的绝对值和，而是各个起始位置连续K个点的各种 min-max 值，最后是一个 [threshold -1, 255]的范围（之所以不求绝对值和，大概是为了防止和的数值太大？）

## 附

1. 又看了下高博的书，书里其实明确写了是 “连续的” 点，自己马虎了。但是 FAST-N 的说法的的确在代码里没体现。
2. 这里 [FAST Corner Detection -- Edward Rosten][fast_original] 可以找到原作者的实现，含 ML, 甚至有 Python 实现……

3. 搜了下自己文章的标题，才发现原来挺多写这个的博客。之前怎么没发现呢…… 另外用 “源码分析” 就搜不到这篇博客，感觉 Google Search 还是有优化空间的 :dog:

[fast_opencv_tutorial]: https://docs.opencv.org/3.4/df/d0c/tutorial_py_fast.html "FAST Algorithm for Corner Detection"
[fast_code]: https://github.com/opencv/opencv/blob/master/modules/features2d/src/fast.cpp
[fast_original]: https://www.edwardrosten.com/work/fast.html 