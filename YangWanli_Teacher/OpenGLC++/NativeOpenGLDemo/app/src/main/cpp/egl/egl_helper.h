#ifndef EGLHELPER_H_
#define EGLHELPER_H_

#include "EGL/egl.h"
#include "../log/android_log.h"

class EglHelper {
public:
    EglHelper();

    ~EglHelper();

public:
    int InitEgl(EGLNativeWindowType window);

    int SwapBuffers();

    void DestroyEgl();

private:
    EGLDisplay m_egl_display_;
    EGLSurface m_egl_surface_;
    EGLConfig m_egl_config_;
    EGLContext m_egl_context_;
};

#endif //EGLHELPER_H_
