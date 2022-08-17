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
