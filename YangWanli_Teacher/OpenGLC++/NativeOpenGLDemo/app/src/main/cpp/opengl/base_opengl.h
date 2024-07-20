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

    virtual void OnChange(int w, int h);

    virtual void OnDraw();

    virtual void SetImagePixel(int image_width, int image_height, void *data);

    virtual void SetYuvData(int width, int height, void *y, void *u, void *v);

    virtual void DestroySource();

    virtual void Destroy();

public:
    int surface_width = 0;//屏幕宽
    int surface_height = 0;//屏幕高

    char *vertexStr = NULL;//顶点着色器代码
    char *fragmentStr = NULL;//片元着色器代码

    float *vertexs = NULL;//顶点坐标
    float *fragments = NULL;//纹理坐标

    GLuint program = 0;//opengl程序id
    GLuint vShader = 0;
    GLuint fShader = 0;
};

#endif //BASE_OPENGL_H_
