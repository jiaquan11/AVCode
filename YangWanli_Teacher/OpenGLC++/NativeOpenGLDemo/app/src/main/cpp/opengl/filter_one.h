#ifndef FILTERONE_H_
#define FILTERONE_H_

#include "base_opengl.h"

/**
 * 绘制纹理
 */
class FilterOne : public BaseOpengl {
public:
    FilterOne();

    ~FilterOne();

public:
    void OnCreate();

    void OnChange(int width, int height);

    void OnDraw();

    void SetImagePixel(int image_width, int image_height, void *data);

    void DestroySource();

    void Destroy();

private:
    void _SetMatrix(int width, int height);

public:
    GLint vPosition = 0;
    GLint fPosition = 0;
    GLint sampler = 0;
    GLuint textureID = 0;
    GLint u_matrix = 0;

    int w = 0;//图片宽
    int h = 0;//图片高
    void *pixels = NULL;//图片像素数据

    float matrix[16] = {0};//用于存放单位矩阵
};

#endif //FILTERONE_H_
