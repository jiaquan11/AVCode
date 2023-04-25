#ifndef _OPENGL_H_
#define _OPENGL_H_

#include "../egl/EglThread.h"
#include "android/native_window.h"
#include "android/native_window_jni.h"
#include "BaseOpengl.h"
#include "FilterOne.h"
#include "FilterTwo.h"
#include "FilterYUV.h"
#include <string>

class Opengl {
public:
    Opengl();

    ~Opengl();

public:
    void onCreateSurface(JNIEnv *env, jobject surface);

    void onChangeSurface(int width, int height);

    void onChangeSurfaceFilter();

    void setPixel(void *data, int width, int height, int length);

    void setYuvData(void *y, void *u, void *v, int w, int h);

    void onDestroySurface();

public:
    EglThread *eglThread = NULL;
    ANativeWindow *nativeWindow = NULL;
    BaseOpengl *baseOpengl = NULL;

    void *pixels = NULL;

    int pic_width = 0;
    int pic_height = 0;
};


#endif
