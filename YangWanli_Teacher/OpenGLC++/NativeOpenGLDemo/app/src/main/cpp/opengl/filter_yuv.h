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

    void Destroy();

    void SetYuvData(int yuv_width, int yuv_height, void *y_data, void *u_data, void *v_data);

private:
    GLint m_sampler_y_ = 0;
    GLint m_sampler_u_ = 0;
    GLint m_sampler_v_ = 0;
    GLuint m_samplers_[3] = {0};
    void *m_y_data_ = NULL;
    void *m_u_data_ = NULL;
    void *m_v_data_ = NULL;
};

#endif //FILTER_YUV_H_
