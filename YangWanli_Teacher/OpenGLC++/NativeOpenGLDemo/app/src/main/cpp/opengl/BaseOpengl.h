#ifndef _BASEOPENGL_H_
#define _BASEOPENGL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GLES2/gl2.h"
#include "../log/androidLog.h"
#include "../matrix/MatrixUtil.h"
#include "../shaderutil/shaderUtil.h"

#define GET_STR(x) #x

class BaseOpengl {
public:
    BaseOpengl();

    ~BaseOpengl();

public:
    virtual void onCreate();

    virtual void onChange(int w, int h);

    virtual void onDraw();

    virtual void setPixel(void *data, int width, int height, int length);

    virtual void setYuvData(void *y, void *u, void *v, int width, int height);

    virtual void destroySource();

    virtual void destroy();

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

#endif
