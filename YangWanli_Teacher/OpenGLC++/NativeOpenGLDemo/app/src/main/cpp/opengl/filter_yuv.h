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
    void OnCreate();

    void OnChange(int w, int h);

    void OnDraw();

    void SetYuvData(int width, int height, void *y, void *u, void *v);

    void DestroySource();

    void Destroy();

private:
    void _SetMatrix(int width, int height);

public:
    GLint vPosition = 0;
    GLint fPosition = 0;
    GLint u_matrix = 0;

    GLint sampler_y = 0;
    GLint sampler_u = 0;
    GLint sampler_v = 0;

    GLuint samplers[3] = {0};
    float matrix[16] = {0};

    void *y_data = NULL;
    void *u_data = NULL;
    void *v_data = NULL;

    int yuv_width = 0;
    int yuv_height = 0;
};

#endif //FILTER_YUV_H_
