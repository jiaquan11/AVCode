#include "opengl.h"

//函数指针的相关实现函数
void callback_SurfaceCreate(void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->m_base_opengl != NULL) {
            opengl->m_base_opengl->onCreate();
        }
    }
}

void callback_SurfaceChange(int width, int height, void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->m_base_opengl != NULL) {
            opengl->m_base_opengl->onChange(width, height);
        }
    }
}

void callback_SurfaceDraw(void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->m_base_opengl != NULL) {
            opengl->m_base_opengl->onDraw();
        }
    }
}

/*切换滤镜
 先把上次的滤镜资源类销毁，然后重新创建新的滤镜绘制类
 */
void callback_SurfaceChangeFilter(int width, int height, void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->m_base_opengl != NULL) {
            opengl->m_base_opengl->destroySource();
            opengl->m_base_opengl->destroy();
            delete opengl->m_base_opengl;
            opengl->m_base_opengl = NULL;
        }

        //切换滤镜操作
        opengl->m_base_opengl = new FilterTwo();
        opengl->m_base_opengl->onCreate();
        opengl->m_base_opengl->onChange(width, height);//屏幕显示宽高
        opengl->m_base_opengl->setPixel(opengl->m_pixels, opengl->m_pic_width, opengl->m_pic_height);

        opengl->m_egl_thread->notifyRender();
    }
}

void callback_SurfaceDestroy(void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->m_base_opengl != NULL) {
            opengl->m_base_opengl->destroy();
        }
    }
}

Opengl::Opengl() {

}

Opengl::~Opengl() {
    if (m_pixels != NULL) {
        free(m_pixels);
        m_pixels = NULL;
    }
}

void Opengl::OnSurfaceCreate(JNIEnv *env, jobject surface) {
    LOGI("Opengl OnSurfaceCreate in");
    m_egl_thread = new EglThread();
    m_egl_thread->setRenderType(OPENGL_RENDER_HANDLE);//设置渲染类型
    m_egl_thread->callBackOnCreate(callback_SurfaceCreate, this);//设置函数指针
    m_egl_thread->callBackOnChange(callback_SurfaceChange, this);
    m_egl_thread->callBackOnDraw(callback_SurfaceDraw, this);
    m_egl_thread->callBackOnChangeFilter(callback_SurfaceChangeFilter, this);
    m_egl_thread->callBackOnDestroy(callback_SurfaceDestroy, this);

    m_base_opengl = new FilterOne();//opengl绘制图片纹理
//    baseOpengl = new FilterYUV();//opengl绘制YUV视频

    m_nativeWindow_ = ANativeWindow_fromSurface(env, surface);
    m_egl_thread->onSurfaceCreate(m_nativeWindow_);//内部创建一个独立的子线程，用于EGL环境的操作
    LOGI("Opengl OnSurfaceCreate end");
}

void Opengl::OnSurfaceChange(int width, int height) {
    LOGI("Opengl OnSurfaceChange in width:%d, height:%d", width, height);
    if (m_egl_thread != NULL) {
        if (m_base_opengl != NULL) {
            m_base_opengl->surface_width = width;
            m_base_opengl->surface_height = height;
        }
        m_egl_thread->onSurfaceChange(width, height);
    }
    LOGI("Opengl OnSurfaceChange end");
}

void Opengl::OnSurfaceDestroy() {
    LOGI("Opengl OnSurfaceDestroy in");
    if (m_egl_thread != NULL) {
        m_egl_thread->destroy();
        delete m_egl_thread;
        m_egl_thread = NULL;
    }
    if (m_base_opengl != NULL) {
        m_base_opengl->destroySource();
        delete m_base_opengl;
        m_base_opengl = NULL;
    }
    /**
     * 释放native surface
     * 避免内存泄漏
     */
    if (m_nativeWindow_ != NULL) {
        ANativeWindow_release(m_nativeWindow_);
        m_nativeWindow_ = NULL;
    }
    if (m_pixels != NULL) {
        free(m_pixels);
        m_pixels = NULL;
    }
    LOGI("Opengl OnSurfaceDestroy end");
}

void Opengl::OnSurfaceChangeFilter() {
    if (m_egl_thread != NULL) {
        m_egl_thread->onSurfaceChangeFilter();
    }
}

void Opengl::SetImgData(int width, int height, int size, void* data) {
    m_pic_width = width;
    m_pic_height = height;
    if (m_pixels != NULL) {
        free(m_pixels);
        m_pixels = NULL;
    }
    m_pixels = malloc(size);
    memcpy(m_pixels, data, size);
    if (m_base_opengl != NULL) {
        m_base_opengl->setPixel(m_pixels, width, height);
    }

    if (m_egl_thread != NULL) {
        m_egl_thread->notifyRender();
    }
}

void Opengl::SetYuvData(void *y, void *u, void *v, int w, int h) {
    if (m_base_opengl != NULL) {
        m_base_opengl->setYuvData(y, u, v, w, h);
    }
    if (m_egl_thread != NULL) {
        m_egl_thread->notifyRender();
    }
}
