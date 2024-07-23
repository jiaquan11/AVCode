#include "egl_thread.h"
#include <stdlib.h>
#include <errno.h>

EglThread::EglThread() {
    pthread_mutex_init(&m_pthread_mutex, NULL);
    pthread_cond_init(&m_pthread_cond, NULL);
}

EglThread::~EglThread() {
    pthread_mutex_destroy(&m_pthread_mutex);
    pthread_cond_destroy(&m_pthread_cond);
}

void EglThread::SetRenderType(int render_type) {
    m_render_type = render_type;
}

void EglThread::SetOnCreateCb(OnCreateCb on_create_cb, void *arg) {
    m_on_create_cb = on_create_cb;
    m_on_create_arg = arg;
}

void EglThread::SetOnChangeCb(OnChangeCb on_change_cb, void *arg) {
    m_on_change_cb = on_change_cb;
    m_on_change_arg = arg;
}

void EglThread::SetOnDrawCb(OnDrawCb on_draw_cb, void *arg) {
    m_on_draw_cb = on_draw_cb;
    m_on_draw_arg = arg;
}

void EglThread::SetOnDestroyCb(OnDestroyCb on_destroy_cb, void *arg) {
    m_on_destroy_cb = on_destroy_cb;
    m_on_destroy_arg = arg;
}

void EglThread::SetOnChangeFilterCb(OnChangeFilterCb on_change_filter_cb, void *arg) {
    m_on_change_filter_cb = on_change_filter_cb;
    m_on_change_filter_arg = arg;
}

void *_EglThreadImpl(void *arg) {
    EglThread *egl_thread = static_cast<EglThread *>(arg);
    if (egl_thread != NULL) {
        EglHelper *egl_helper = new EglHelper();
        egl_helper->InitEgl(egl_thread->m_native_window);
        egl_thread->m_is_exit = false;

        while (true) {
            if (egl_thread->m_is_create) {
                LOGI("egl_thread call surface create!");
                egl_thread->m_is_create = false;
                egl_thread->m_on_create_cb(egl_thread->m_on_create_arg);
            }

            if (egl_thread->m_is_change) {
                LOGI("egl_thread call surface change!");
                egl_thread->m_is_change = false;
                egl_thread->m_on_change_cb(egl_thread->m_surface_width, egl_thread->m_surface_height, egl_thread->m_on_change_arg);
                egl_thread->m_is_start = true;
            }

            if (egl_thread->m_is_change_filter) {
                egl_thread->m_is_change_filter = false;
                egl_thread->m_on_change_filter_cb(egl_thread->m_surface_width, egl_thread->m_surface_height, egl_thread->m_on_change_filter_arg);
            }

            if (egl_thread->m_is_start) {
                egl_thread->m_on_draw_cb(egl_thread->m_on_draw_arg);
                egl_helper->SwapBuffers();
            }

            if (egl_thread->m_render_type == OPENGL_RENDER_AUTO) {//自动渲染
                usleep(1000000 / 60);//六十分之一秒  每秒60次渲染
            } else {//手动渲染
                pthread_mutex_lock(&egl_thread->m_pthread_mutex);
                pthread_cond_wait(&egl_thread->m_pthread_cond, &egl_thread->m_pthread_mutex);
                pthread_mutex_unlock(&egl_thread->m_pthread_mutex);
            }
            if (egl_thread->m_is_exit) {
                egl_thread->m_on_destroy_cb(egl_thread->m_on_destroy_arg);
                egl_helper->DestroyEgl();
                delete egl_helper;
                break;
            }
        }
    }
    return 0;
}

void EglThread::OnSurfaceCreate(EGLNativeWindowType window) {
    if (m_egl_thread_ == -1) {
        m_is_create = true;
        m_native_window = window;
        int ret = pthread_create(&m_egl_thread_, NULL, _EglThreadImpl, this);
        if (ret != 0) {
            LOGE("pthread_create m_egl_thread_ error, ret = %d", ret);
            m_egl_thread_ = -1;
        }
    }
}

void EglThread::OnSurfaceChange(int surface_width, int surface_height) {
    m_is_change = true;
    m_surface_width = surface_width;
    m_surface_height = surface_height;
    NotifyRender();
}

void EglThread::OnSurfaceDestroy() {
    m_is_exit = true;
    NotifyRender();

    void *thread_ret;
    int result = pthread_join(m_egl_thread_, &thread_ret);
    LOGI("m_egl_thread_ join result: %d", result);
    if (result != 0) {
        switch (result) {
            case ESRCH:
                LOGE("m_egl_thread_ pthread_join failed: Thread not found (ESRCH)");
                break;
            case EINVAL:
                LOGE("m_egl_thread_ pthread_join failed: Invalid thread or thread already detached (EINVAL)");
                break;
            case EDEADLK:
                LOGE("m_egl_thread_ pthread_join failed: Deadlock detected (EDEADLK)");
                break;
            default:
                LOGE("m_egl_thread_ pthread_join failed: Unknown error (%d)", result);
        }
        // Exit or perform additional actions if needed
        exit(EXIT_FAILURE);
    } else {
        LOGI("m_egl_thread_ Thread returned: %ld", (long)thread_ret);
    }
    m_native_window = NULL;
    m_egl_thread_ = -1;
}

void EglThread::OnSurfaceChangeFilter() {
    m_is_change_filter = true;
    NotifyRender();
}

void EglThread::NotifyRender() {
    pthread_mutex_lock(&m_pthread_mutex);
    pthread_cond_signal(&m_pthread_cond);
    pthread_mutex_unlock(&m_pthread_mutex);
}