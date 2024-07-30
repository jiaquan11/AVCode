#ifndef FILTER_TWO_H_
#define FILTER_TWO_H_

#include "base_opengl.h"

/**
 * 切换滤镜
 */
class FilterTwo : public BaseOpengl {
public:
    FilterTwo();

    ~FilterTwo();

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

#endif //FILTER_TWO_H_
