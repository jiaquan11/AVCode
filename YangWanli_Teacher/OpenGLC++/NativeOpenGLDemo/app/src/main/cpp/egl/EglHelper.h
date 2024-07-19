#ifndef _EGLHELPER_H_
#define _EGLHELPER_H_

#include "EGL/egl.h"
#include "../log/android_log.h"

class EglHelper {
public:
    EglHelper();

    ~EglHelper();

public:
    int initEgl(EGLNativeWindowType window);

    int swapBuffers();

    void destroyEgl();

public:
    EGLDisplay mEglDisplay;
    EGLSurface mEglSurface;
    EGLConfig mEglConfig;
    EGLContext mEglContext;
};

#endif
