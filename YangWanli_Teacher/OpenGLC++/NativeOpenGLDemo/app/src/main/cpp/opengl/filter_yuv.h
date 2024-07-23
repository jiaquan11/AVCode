#ifndef FILTER_YUV_H_
#define FILTER_YUV_H_

#include "base_opengl.h"

/**
 * 绘制YUV图像
 */
class FilterYUV : public BaseOpengl {
public:
    FilterYUV();

    ~FilterYUV();

public:
    void OnCreate();

    void OnChange(int surface_width, int surface_height);

    void OnDraw();

    void DestroySource();

    void Destroy();

    void SetYuvData(int yuv_width, int yuv_height, void *y_data, void *u_data, void *v_data);

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



private:
    int m_yuv_width_ = 0;
    int m_yuv_height_ = 0;
    void *m_y_data_ = NULL;
    void *m_u_data_ = NULL;
    void *m_v_data_ = NULL;
};

#endif //FILTER_YUV_H_
