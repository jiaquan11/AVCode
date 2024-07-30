#ifndef EGL_THREAD_H_
#define EGL_THREAD_H_

#include <EGL/eglplatform.h>
#include "pthread.h"
#include "android/native_window.h"
#include <unistd.h>
#include <GLES2/gl2.h>

#include "egl_helper.h"

#define OPENGL_RENDER_AUTO 1
#define OPENGL_RENDER_HANDLE 2

class EglThread {
public:
    typedef void(*OnCreateCb)(void *);
    OnCreateCb m_on_create_cb;
    void *m_on_create_arg;

    typedef void(*OnChangeCb)(void *);
    OnChangeCb m_on_change_cb;
    void *m_on_change_arg;

    typedef void(*OnDrawCb)(void *);
    OnDrawCb m_on_draw_cb;
    void *m_on_draw_arg;

    typedef void(*OnDestroyCb)(void *);
    OnDestroyCb m_on_destroy_cb;
    void *m_on_destroy_arg;

    typedef void(*OnChangeFilterCb)(void *);
    OnChangeFilterCb m_on_change_filter_cb;
    void *m_on_change_filter_arg;

public:
    EglThread();

    ~EglThread();

public:
    void SetRenderType(int render_type);

    void SetOnCreateCb(OnCreateCb on_create_cb, void *arg);

    void SetOnChangeCb(OnChangeCb on_change_cb, void *arg);

    void SetOnDrawCb(OnDrawCb on_draw_cb, void *arg);

    void SetOnDestroyCb(OnDestroyCb on_destroy_cb, void *arg);

    void SetOnChangeFilterCb(OnChangeFilterCb on_change_filter_cb, void *arg);

    void OnSurfaceCreate(EGLNativeWindowType window);

    void OnSurfaceChange();

    void OnSurfaceDestroy();

    void OnSurfaceChangeFilter();

    void NotifyRender();

public:
    pthread_mutex_t m_pthread_mutex;
    pthread_cond_t m_pthread_cond;
    ANativeWindow *m_native_window = NULL;
    int m_render_type = OPENGL_RENDER_AUTO;
    bool m_is_create = false;
    bool m_is_change = false;
    bool m_is_start = false;
    bool m_is_change_filter = false;
    bool m_is_exit = false;

private:
    pthread_t m_egl_thread_ = -1;
};

#endif //EGL_THREAD_H_
