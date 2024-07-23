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

#endif //FILTER_ONE_H_
