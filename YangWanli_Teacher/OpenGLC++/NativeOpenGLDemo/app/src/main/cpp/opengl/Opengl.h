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

    void OnSurfaceChange(int surface_width, int surface_height);

    void OnSurfaceDestroy();

    void OnSurfaceChangeFilter();

    void SetImgData(int image_width, int image_height, int size, void* data);

    void SetYuvData(int yuv_width, int yuv_height, void *y_data, void *u_data, void *v_data);

public:
    EglThread *m_egl_thread = NULL;
    BaseOpengl *m_base_opengl = NULL;//opengl渲染器的基类，可以派生出不同的渲染器
    void *m_image_pixels = NULL;
    int m_image_width = 0;
    int m_image_height = 0;

private:
    ANativeWindow *m_native_window_ = NULL;
};

#endif //OPENGL_H_
