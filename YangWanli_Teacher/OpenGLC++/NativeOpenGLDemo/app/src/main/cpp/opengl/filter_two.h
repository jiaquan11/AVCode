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

    void DestroySource();

    void Destroy();

    void SetImagePixel(int image_width, int image_height, void *data);

private:
    void _SetMatrix(int width, int height);

public:
    GLint vPosition = 0;
    GLint fPosition = 0;
    GLint sampler = 0;
    GLuint textureID = 0;
    GLint u_matrix = 0;
    void *pixels = NULL;
    float matrix[16] = {0};

private:
    int m_image_width_ = 0;
    int m_image_height_ = 0;
};

#endif //FILTER_TWO_H_
