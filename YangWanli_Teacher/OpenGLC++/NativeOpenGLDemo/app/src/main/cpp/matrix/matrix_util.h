#ifndef MATRIX_UTIL_H_
#define MATRIX_UTIL_H_

#include <math.h>

/**
 * 矩阵操作，调用示例
 *InitMatrix(matrix);//初始化矩阵
 *RotateMatrix(90, matrix);//旋转矩阵，这里是逆时针90度旋转，-90是顺时针旋转90度
 *ScaleMatrix(0.5, matrix);//图像缩放，一般是对X轴和Y轴等值缩放，0.5缩小一倍
 *TransMatrix(0.5, 0, matrix);//图像平移，这里移动二分之一位置，沿着X轴
 *TransMatrix(1, 1, matrix);
 *根据顶点坐标的范围-1，1，-1，1，表示全屏幕铺满
 *OrthoM(-1, 1, -1, 1, matrix);//正交投影
*/

/**
 * 初始化矩阵(4*4矩阵)
 * 对角线数字为1，其余为0
 */
static void InitMatrix(float *matrix) {
    for (int i = 0; i < 16; ++i) {
        if (i % 5 == 0) {
            matrix[i] = 1;
        } else {
            matrix[i] = 0;
        }
    }
}

/**
 * 2D旋转:2D图像旋转都是沿着Z轴旋转
 * angle为正数，表示逆时针旋转，为负数表示顺时针旋转
 */
static void RotateMatrix(double angle, float *matrix) {
    angle = angle * (M_PI / 180.0);//将角度转为弧度
    //修改矩阵中的部分特定值，表示沿着Z轴旋转
    matrix[0] = cos(angle);
    matrix[1] = -sin(angle);
    matrix[4] = sin(angle);
    matrix[5] = cos(angle);
}

/**
 *2D缩放:缩放一般是均匀缩放
 *一般是对X轴和Y轴同值缩放，Z轴默认是1
 */
static void ScaleMatrix(double scale, float *matrix) {
    matrix[0] = scale;
    matrix[5] = scale;
}

/**
 *2D平移:只需修改X轴和Y轴即可
 */
static void TransMatrix(double x, double y, float *matrix) {
    matrix[3] = x;
    matrix[7] = y;
}

/**
 * 正交投影
 */
static void OrthoM(float left, float right, float bottom, float top, float *matrix) {
    matrix[0] = 2 / (right - left);
    matrix[3] = (right + left) / (right - left) * -1;
    matrix[5] = 2 / (top - bottom);
    matrix[7] = (top + bottom) / (top - bottom) * -1;
    matrix[10] = 1;
    matrix[11] = 1;
}

#endif //MATRIX_UTIL_H_