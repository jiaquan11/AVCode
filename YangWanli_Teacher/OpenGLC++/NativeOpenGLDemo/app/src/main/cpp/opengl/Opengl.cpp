#include "opengl.h"

//函数指针的相关实现函数
void SurfaceCreateCb(void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->m_base_opengl != NULL) {
            opengl->m_base_opengl->OnCreate();
        }
    }
}

void SurfaceChangeCb(int width, int height, void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->m_base_opengl != NULL) {
            opengl->m_base_opengl->OnChange(width, height);
        }
    }
}

void SurfaceDrawCb(void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->m_base_opengl != NULL) {
            opengl->m_base_opengl->OnDraw();
        }
    }
}

/**
 * 切换滤镜:先把上次的滤镜资源类销毁，然后重新创建新的滤镜绘制类
 */
void SurfaceChangeFilterCb(int width, int height, void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->m_base_opengl != NULL) {
            opengl->m_base_opengl->DestroySource();
            opengl->m_base_opengl->Destroy();
            delete opengl->m_base_opengl;
            opengl->m_base_opengl = NULL;
        }

        //切换滤镜操作
        opengl->m_base_opengl = new FilterTwo();
        opengl->m_base_opengl->OnCreate();
        opengl->m_base_opengl->OnChange(width, height);//屏幕显示宽高
        opengl->m_base_opengl->SetImagePixel(opengl->m_pic_width, opengl->m_pic_height, opengl->m_image_pixels);

        opengl->m_egl_thread->NotifyRender();
    }
}

void SurfaceDestroyCb(void *ctx) {
    Opengl *opengl = static_cast<Opengl *>(ctx);
    if (opengl != NULL) {
        if (opengl->m_base_opengl != NULL) {
            opengl->m_base_opengl->Destroy();
        }
    }
}

Opengl::Opengl() {

}

Opengl::~Opengl() {
    if (m_image_pixels != NULL) {
        free(m_image_pixels);
        m_image_pixels = NULL;
    }
}

void Opengl::OnSurfaceCreate(JNIEnv *env, jobject surface) {
    LOGI("Opengl OnSurfaceCreate in");
    m_egl_thread = new EglThread();
    m_egl_thread->SetRenderType(OPENGL_RENDER_HANDLE);//设置渲染类型
    m_egl_thread->SetOnCreateCb(SurfaceCreateCb, this);//设置函数指针
    m_egl_thread->SetOnChangeCb(SurfaceChangeCb, this);
    m_egl_thread->SetOnDrawCb(SurfaceDrawCb, this);
    m_egl_thread->SetOnChangeFilterCb(SurfaceChangeFilterCb, this);
    m_egl_thread->SetOnDestroyCb(SurfaceDestroyCb, this);

    m_base_opengl = new FilterOne();//opengl绘制图片纹理
//    baseOpengl = new FilterYUV();//opengl绘制YUV视频

    m_nativeWindow_ = ANativeWindow_fromSurface(env, surface);
    m_egl_thread->OnSurfaceCreate(m_nativeWindow_);//内部创建一个独立的子线程，用于EGL环境的操作
    LOGI("Opengl OnSurfaceCreate end");
}

void Opengl::OnSurfaceChange(int width, int height) {
    LOGI("Opengl OnSurfaceChange in width:%d, height:%d", width, height);
    if (m_egl_thread != NULL) {
        if (m_base_opengl != NULL) {
            m_base_opengl->surface_width = width;
            m_base_opengl->surface_height = height;
        }
        m_egl_thread->OnSurfaceChange(width, height);
    }
    LOGI("Opengl OnSurfaceChange end");
}

void Opengl::OnSurfaceDestroy() {
    LOGI("Opengl OnSurfaceDestroy in");
    if (m_egl_thread != NULL) {
        m_egl_thread->OnSurfaceDestroy();
        delete m_egl_thread;
        m_egl_thread = NULL;
    }
    if (m_base_opengl != NULL) {
        m_base_opengl->DestroySource();
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
    if (m_image_pixels != NULL) {
        free(m_image_pixels);
        m_image_pixels = NULL;
    }
    LOGI("Opengl OnSurfaceDestroy end");
}

void Opengl::OnSurfaceChangeFilter() {
    if (m_egl_thread != NULL) {
        m_egl_thread->OnSurfaceChangeFilter();
    }
}

void Opengl::SetImgData(int image_width, int image_height, int size, void* data) {
    m_pic_width = image_width;
    m_pic_height = image_height;
    if (m_image_pixels != NULL) {
        free(m_image_pixels);
        m_image_pixels = NULL;
    }
    m_image_pixels = malloc(size);
    memcpy(m_image_pixels, data, size);
    if (m_base_opengl != NULL) {
        m_base_opengl->SetImagePixel(image_width, image_height, m_image_pixels);
    }
    if (m_egl_thread != NULL) {
        m_egl_thread->NotifyRender();
    }
}

void Opengl::SetYuvData(void *y, void *u, void *v, int w, int h) {
    if (m_base_opengl != NULL) {
        m_base_opengl->SetYuvData(w, h, y, u, v);
    }
    if (m_egl_thread != NULL) {
        m_egl_thread->NotifyRender();
    }
}
