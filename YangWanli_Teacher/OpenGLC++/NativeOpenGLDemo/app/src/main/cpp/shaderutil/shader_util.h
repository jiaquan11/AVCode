#ifndef SHADER_UTIL_H_
#define SHADER_UTIL_H_

#include <GLES2/gl2.h>
#include "../log/android_log.h"

static void CheckGLError() {
    GLenum error = glGetError();
    LOGE("CheckGLError error:%d", error);
    if (error != GL_NO_ERROR) {
        switch (error) {
            case GL_INVALID_ENUM:
                LOGE("CheckGLError GL_INVALID_ENUM");
                break;
            case GL_INVALID_VALUE:
                LOGE("CheckGLError GL_INVALID_VALUE");
                break;
            case GL_INVALID_OPERATION:
                LOGE("CheckGLError GL_INVALID_OPERATION");
                break;
            case GL_OUT_OF_MEMORY:
                LOGE("CheckGLError GL_OUT_OF_MEMORY");
                break;
            default:
                LOGE("CheckGLError default");
                break;
        }
    }
}

static int LoadShaders(int shader_type, const char *code) {
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &code, 0);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {//1表示成功，0表示失败
        LOGE("glCompileShader failed!");
        return 0;
    }
    LOGI("glCompileShader success!");
    return shader;
}

static int CreateProgram(const char *vertex, const char *fragment, GLuint *v_shader, GLuint *f_shader) {
    //1。加载顶点着色器和片元着色器
    GLuint vertex_shader = LoadShaders(GL_VERTEX_SHADER, vertex);
    LOGI("vertexShader is %d", vertex_shader);
    GLuint fragment_shader = LoadShaders(GL_FRAGMENT_SHADER, fragment);
    LOGI("fragmentShader is %d", fragment_shader);
    //2.创建一个渲染程序
    int program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    //3.链接渲染程序
    glLinkProgram(program);

    *v_shader = vertex_shader;
    *f_shader = fragment_shader;
    //4.检查链接状态
    GLint status;
    glGetShaderiv(program, GL_LINK_STATUS, &status);
    if (status == 0) {
        LOGE("glLinkProgram failed!");
        return 0;
    }
    LOGI("glLinkProgram success!");
    return program;
}

#endif //SHADER_UTIL_H_
