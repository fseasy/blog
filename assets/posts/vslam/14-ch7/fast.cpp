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
    const int K = patternSize/2, N = patternSize + K + 1;
    int i, j, k, pixel[25];
    // img.step 表示图像的一行需要的字节大小。
    // makeOffsets 实现在自 fast_score.cpp 中；
    // 签名为 void makeOffsets(int pixel[25], int rowStride, int patternSize)
    // 功能是提前算好周围元素相对与中心点的偏移量（一维、字节层面）。
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

    // buf: （行）像素缓冲区； 这里申请了3行； 注意到type 是 uchar, 故是只能处理 8UC1 的图像（即灰度图）的！
    // cpbuf: （行）角点位置缓冲区， 从后面可知为 CornerPos buf 的缩写；下面看到每行的 size 为 cols + 1, 
    //        比 row-size 多申请了 1 个， 具体原因可看后面
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
        // curr：  xxx，赋值为 buf 的第 0 维数组（长度为 cols = row size）； 
        //         (i - 3) %3 使得消除 i 从 3 开始的偏移，且保证取值在 {0，1，2} 中 ( 前面只申请了 3 个长度)
        uchar* curr = buf[(i - 3)%3];
        // cornerpos 同理； 注意， 从 cornerpos 可以推出 cpbuf 就是 CornerPos 的 buf, 即角点位置的缓冲区，所以类型是int
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
            j = 3;
            for( ; j < img.cols - 3; j++, ptr++ )
            {
                // ptr 初始是 当前行第 4 列；这里将此位置的像素值取出赋给v； 
                //     这里有隐式类型转换（提升）: uchar -> int; 安全的
                int v = ptr[0];
                const uchar* tab = &threshold_tab[0] - v + 255;
                int d = tab[ptr[pixel[0]]] | tab[ptr[pixel[8]]];

                if( d == 0 )
                    continue;

                d &= tab[ptr[pixel[2]]] | tab[ptr[pixel[10]]];
                d &= tab[ptr[pixel[4]]] | tab[ptr[pixel[12]]];
                d &= tab[ptr[pixel[6]]] | tab[ptr[pixel[14]]];

                if( d == 0 )
                    continue;

                d &= tab[ptr[pixel[1]]] | tab[ptr[pixel[9]]];
                d &= tab[ptr[pixel[3]]] | tab[ptr[pixel[11]]];
                d &= tab[ptr[pixel[5]]] | tab[ptr[pixel[13]]];
                d &= tab[ptr[pixel[7]]] | tab[ptr[pixel[15]]];

                if( d & 1 )
                {
                    int vt = v - threshold, count = 0;

                    for( k = 0; k < N; k++ )
                    {
                        int x = ptr[pixel[k]];
                        if(x < vt)
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

                if( d & 2 )
                {
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
            }
        }

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
