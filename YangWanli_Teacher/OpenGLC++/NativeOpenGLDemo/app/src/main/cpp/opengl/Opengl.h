#ifndef OPENGL_H_
#define OPENGL_H_

#include "android/native_window.h"
#include "android/native_window_jni.h"
#include "../egl/egl_thread.h"
#include "base_opengl.h"
#include "filter_one.h"
#include "filter_two.h"
#include "filter_yuv.h"

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
    EglThread *m_egl_thread = NULL;
    BaseOpengl *m_base_opengl = NULL;
    void *m_pixels = NULL;
    int m_pic_width = 0;
    int m_pic_height = 0;

private:
    ANativeWindow *m_nativeWindow_ = NULL;
};

#endif //OPENGL_H_
