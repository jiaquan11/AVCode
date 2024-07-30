#ifndef FILTER_ONE_H_
#define FILTER_ONE_H_

#include "base_opengl.h"

/**
 * 绘制图片纹理
 */
class FilterOne : public BaseOpengl {
public:
    FilterOne();

    ~FilterOne();

public:
    void OnCreate();

    void OnChange(int surface_width, int surface_height);

    void OnDraw();

    void Destroy();

    void SetImagePixel(int image_width, int image_height, void *pixels);

private:
    GLint m_sampler_ = 0;
    GLuint m_texture_id_ = 0;
    void *m_pixels_ = NULL;
};

#endif //FILTER_ONE_H_
