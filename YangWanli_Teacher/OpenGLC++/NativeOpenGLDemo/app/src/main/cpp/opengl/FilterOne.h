#ifndef _FILTERONE_H_
#define _FILTERONE_H_

#include "BaseOpengl.h"

/*
 * 绘制纹理
 * */
class FilterOne : public BaseOpengl {
public:
    FilterOne();

    ~FilterOne();

public:
    void onCreate();

    void onChange(int w, int h);

    void onDraw();

    void setPixel(void *data, int width, int height);

    void destroySource();

    void destroy();

private:
    void setMatrix(int width, int height);

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

#endif
