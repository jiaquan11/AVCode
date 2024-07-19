#ifndef FILTER_YUV_H_
#define FILTER_YUV_H_

#include "base_opengl.h"

/*
 * 绘制YUV图像
 * */
class FilterYUV : public BaseOpengl {
public:
    FilterYUV();

    ~FilterYUV();

public:
    void onCreate();

    void onChange(int w, int h);

    void onDraw();

    void setYuvData(void *Y, void *U, void *V, int width, int height);

    void destroySource();

    void destroy();

private:
    void setMatrix(int width, int height);

public:
    GLint vPosition = 0;
    GLint fPosition = 0;
    GLint u_matrix = 0;

    GLint sampler_y = 0;
    GLint sampler_u = 0;
    GLint sampler_v = 0;

    GLuint samplers[3] = {0};
    float matrix[16] = {0};

    void *y = NULL;
    void *u = NULL;
    void *v = NULL;

    int yuv_width = 0;
    int yuv_height = 0;
};

#endif //FILTER_YUV_H_
