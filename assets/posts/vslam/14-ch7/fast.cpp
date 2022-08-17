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

    // buf: （行）缓冲区； 由后可知，是放nomax-suppression时要用的分数(uchar即可，因为分数在灰度区间内)
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
        // img.ptr<T> 是 OpenCV 中迭代像素（差不多）最快的方式
        // 它按行迭代， .ptr<T>(i) 就是得到第 i 行元素的指针； 访问第 i 行的第j列，直接用C++原生的下标操作 [j] 即可
        // --- PS: 对(数组）指针a， a[j] 就是得到第j个元素的地址 addr = a + j * sizeof(*a)
        // .ptr(i) 与 .at(i) 都是访问第 i 行头地址，但是 .at 有边界检查，所以会慢一点……
        // 因此下面的 ptr， 就是图像 (i, 3) 位置的像素地址！ 
        // 结合 i 从 row = 3 开始，可知迭代都是从 第4行(0-based)、第4列 开始的！
        const uchar* ptr = img.ptr<uchar>(i) + 3;
        // curr：由后可知，存储是当前行个角点的分数（用于nomax-suppression)，
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
                                // 如果需要做 nomax-suppression (非极大值抑制)，就算当前中心点的分
                                //    这个分，必然是一个正数，且范围在 [treshold - 1, 255] 间，故 uchar 就可以承接
                                // cornerScore的函数签名为
                                //   int cornerScore<SZ>(const uchar* ptr, const int pixel[], int threshold)
                                // ptr 在循环内，是中心点在矩阵的偏移； pixel 是周围点的相对偏移； 
                                // threshold 即输入的灰度阈值
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

        if( i == 3 )
            continue;

        const uchar* prev = buf[(i - 4 + 3)%3];
        const uchar* pprev = buf[(i - 5 + 3)%3];
        cornerpos = cpbuf[(i - 4 + 3)%3] + 1; // cornerpos[-1] is used to store a value
        ncorners = cornerpos[-1];

        for( k = 0; k < ncorners; k++ )
        {
            j = cornerpos[k];
            int score = prev[j];
            if( !nonmax_suppression ||
               (score > prev[j+1] && score > prev[j-1] &&
                score > pprev[j-1] && score > pprev[j] && score > pprev[j+1] &&
                score > curr[j-1] && score > curr[j] && score > curr[j+1]) )
            {
                keypoints.push_back(KeyPoint((float)j, (float)(i-1), 7.f, -1, (float)score));
            }
        }
    }
}

static inline int hal_FAST(cv::Mat& src, std::vector<KeyPoint>& keypoints, int threshold, bool nonmax_suppression, FastFeatureDetector::DetectorType type)
{
    if (threshold > 20)
        return CV_HAL_ERROR_NOT_IMPLEMENTED;

    cv::Mat scores(src.size(), src.type());

    int error = cv_hal_FAST_dense(src.data, src.step, scores.data, scores.step, src.cols, src.rows, type);

    if (error != CV_HAL_ERROR_OK)
        return error;

    cv::Mat suppressedScores(src.size(), src.type());

    if (nonmax_suppression)
    {
        error = cv_hal_FAST_NMS(scores.data, scores.step, suppressedScores.data, suppressedScores.step, scores.cols, scores.rows);

        if (error != CV_HAL_ERROR_OK)
            return error;
    }
    else
    {
        suppressedScores = scores;
    }

    if (!threshold && nonmax_suppression) threshold = 1;

    cv::KeyPoint kpt(0, 0, 7.f, -1, 0);

    unsigned uthreshold = (unsigned) threshold;

    int ofs = 3;

    int stride = (int)suppressedScores.step;
    const unsigned char* pscore = suppressedScores.data;

    keypoints.clear();

    for (int y = ofs; y + ofs < suppressedScores.rows; ++y)
    {
        kpt.pt.y = (float)(y);
        for (int x = ofs; x + ofs < suppressedScores.cols; ++x)
        {
            unsigned score = pscore[y * stride + x];
            if (score > uthreshold)
            {
                kpt.pt.x = (float)(x);
                kpt.response = (nonmax_suppression != 0) ? (float)((int)score - 1) : 0.f;
                keypoints.push_back(kpt);
            }
        }
    }

    return CV_HAL_ERROR_OK;
}

// 最主要的入口函数。 下面的类、少参数的版本，核心都是这个函数。
// 这个函数，最基础的cpu实现，就是前面的 FAST_t 函数
void FAST(InputArray _img, std::vector<KeyPoint>& keypoints, int threshold, bool nonmax_suppression, FastFeatureDetector::DetectorType type)
{
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


void FAST(InputArray _img, std::vector<KeyPoint>& keypoints, int threshold, bool nonmax_suppression)
{
    CV_INSTRUMENT_REGION();

    FAST(_img, keypoints, threshold, nonmax_suppression, FastFeatureDetector::TYPE_9_16);
}


class FastFeatureDetector_Impl CV_FINAL : public FastFeatureDetector
{
public:
    FastFeatureDetector_Impl( int _threshold, bool _nonmaxSuppression, FastFeatureDetector::DetectorType _type )
    : threshold(_threshold), nonmaxSuppression(_nonmaxSuppression), type(_type)
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

    void set(int prop, double value)
    {
        if(prop == THRESHOLD)
            threshold = cvRound(value);
        else if(prop == NONMAX_SUPPRESSION)
            nonmaxSuppression = value != 0;
        else if(prop == FAST_N)
            type = static_cast<FastFeatureDetector::DetectorType>(cvRound(value));
        else
            CV_Error(Error::StsBadArg, "");
    }

    double get(int prop) const
    {
        if(prop == THRESHOLD)
            return threshold;
        if(prop == NONMAX_SUPPRESSION)
            return nonmaxSuppression;
        if(prop == FAST_N)
            return static_cast<int>(type);
        CV_Error(Error::StsBadArg, "");
        return 0;
    }

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

Ptr<FastFeatureDetector> FastFeatureDetector::create( int threshold, bool nonmaxSuppression, FastFeatureDetector::DetectorType type )
{
    return makePtr<FastFeatureDetector_Impl>(threshold, nonmaxSuppression, type);
}

String FastFeatureDetector::getDefaultName() const
{
    return (Feature2D::getDefaultName() + ".FastFeatureDetector");
}

}
