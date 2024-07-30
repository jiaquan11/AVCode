#ifndef BASE_OPENGL_H_
#define BASE_OPENGL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GLES2/gl2.h"
#include "../log/android_log.h"
#include "../matrix/matrix_util.h"
#include "../shaderutil/shader_util.h"

#define GET_STR(x) #x  //将传入的参数转换为字符串

class BaseOpengl {
public:
    BaseOpengl();

    ~BaseOpengl();

public:
    virtual void OnCreate();

    virtual void OnChange(int surface_width, int surface_height);

    virtual void OnDraw();

    virtual void Destroy();

    virtual void SetImagePixel(int image_width, int image_height, void *pixels);

    virtual void SetYuvData(int yuv_width, int yuv_height, void *y_data, void *u_data, void *v_data);

protected:
    void SetMatrix();

protected:
    char *m_vertex_str = NULL;
    char *m_fragment_str = NULL;
    float *m_vertex_array = NULL;
    float *m_fragment_array = NULL;
    float m_matrix[16] = {0};
    GLuint m_vshader = 0;
    GLuint m_fshader = 0;
    GLuint m_program = 0;
    GLint m_v_position = 0;
    GLint m_f_position = 0;
    GLint m_u_matrix = 0;
    int m_surface_width = 0;
    int m_surface_height = 0;
    int m_image_width = 0;
    int m_image_height = 0;
};

#endif //BASE_OPENGL_H_
