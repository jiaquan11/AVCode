#ifndef BASE_OPENGL_H_
#define BASE_OPENGL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GLES2/gl2.h"
#include "../log/android_log.h"
#include "../matrix/matrix_util.h"
#include "../shaderutil/shader_util.h"

#define GET_STR(x) #x

class BaseOpengl {
public:
    BaseOpengl();

    ~BaseOpengl();

public:
    virtual void OnCreate();

    virtual void OnChange(int surface_width, int surface_height);

    virtual void OnDraw();

    virtual void DestroySource();

    virtual void Destroy();

    virtual void SetImagePixel(int image_width, int image_height, void *data);

    virtual void SetYuvData(int yuv_width, int yuv_height, void *y_data, void *u_data, void *v_data);

public:
    int m_surface_width = 0;
    int m_surface_height = 0;

    char *vertexStr = NULL;
    char *fragmentStr = NULL;

    float *vertexs = NULL;
    float *fragments = NULL;

    GLuint program = 0;
    GLuint vShader = 0;
    GLuint fShader = 0;
};

#endif //BASE_OPENGL_H_
