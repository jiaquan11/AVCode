#ifndef OPENGL_H_
#define OPENGL_H_

#include "android/native_window.h"
#include "android/native_window_jni.h"
#include "../egl/EglThread.h"
#include "BaseOpengl.h"
#include "FilterOne.h"
#include "FilterTwo.h"
#include "FilterYUV.h"

class Opengl {
public:
    Opengl();

    ~Opengl();

public:
    void OnSurfaceCreate(JNIEnv *env, jobject surface);

    void OnSurfaceChange(int width, int height);

    void OnSurfaceDestroy();

    void OnSurfaceChangeFilter();

    void SetImgData(int width, int height, int size, void* data);

    void SetYuvData(void *y, void *u, void *v, int w, int h);

public:
    EglThread *eglThread = NULL;
    ANativeWindow *nativeWindow = NULL;
    BaseOpengl *baseOpengl = NULL;

    void *pixels = NULL;

    int pic_width = 0;
    int pic_height = 0;
};

#endif //OPENGL_H_
