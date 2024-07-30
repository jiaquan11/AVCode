#include "opengl.h"

/**
 * 自定义EGL线程:
 * 用于在这个线程中执行某个实例对象的GL操作,保证GL操作在同一个线程中执行
 * 从而避免多线程操作GL资源导致的问题
 * 如下函数指针就是可以灵活的创建实例对象，并在EGL线程中执行某个实例对象的GL操作
 */
//函数指针的相关实现函数
void SurfaceCreateCb(void *arg) {
    Opengl *opengl = static_cast<Opengl *>(arg);
    if ((opengl != NULL) && (opengl->m_base_opengl != NULL)) {
        opengl->m_base_opengl->OnCreate();
    }
}

void SurfaceChangeCb(void *arg) {
    Opengl *opengl = static_cast<Opengl *>(arg);
    if ((opengl != NULL) && (opengl->m_base_opengl != NULL)) {
        opengl->m_base_opengl->OnChange(opengl->m_surface_width, opengl->m_surface_height);
    }
}

void SurfaceDrawCb(void *arg) {
    Opengl *opengl = static_cast<Opengl *>(arg);
    if ((opengl != NULL) && (opengl->m_base_opengl != NULL)) {
        opengl->m_base_opengl->OnDraw();
    }
}

void SurfaceDestroyCb(void *arg) {
    Opengl *opengl = static_cast<Opengl *>(arg);
    if ((opengl != NULL) && (opengl->m_base_opengl != NULL)) {
        opengl->m_base_opengl->Destroy();//释放opengl资源
    }
}

/**
 * 切换滤镜:先把上次的滤镜资源类销毁，然后重新创建新的滤镜绘制类
 */
void SurfaceChangeFilterCb(void *arg) {
    Opengl *opengl = static_cast<Opengl *>(arg);
    if (opengl != NULL) {
        if (opengl->m_base_opengl != NULL) {
            opengl->m_base_opengl->Destroy();
            delete opengl->m_base_opengl;
            opengl->m_base_opengl = NULL;
        }
        //切换滤镜实例对象
        opengl->m_base_opengl = new FilterTwo();
        opengl->m_base_opengl->OnCreate();
        opengl->m_base_opengl->OnChange(opengl->m_surface_width, opengl->m_surface_height);
        opengl->m_base_opengl->SetImagePixel(opengl->m_image_width, opengl->m_image_height, opengl->m_image_pixels);
        opengl->m_egl_thread->NotifyRender();
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
    m_egl_thread->SetOnDestroyCb(SurfaceDestroyCb, this);
    m_egl_thread->SetOnChangeFilterCb(SurfaceChangeFilterCb, this);

    m_base_opengl = new FilterOne();//opengl绘制图片纹理
//    m_base_opengl = new FilterYUV();//opengl绘制YUV视频

    m_native_window_ = ANativeWindow_fromSurface(env, surface);
    m_egl_thread->OnSurfaceCreate(m_native_window_);//内部创建egl环境子线程
    LOGI("Opengl OnSurfaceCreate end");
}

void Opengl::OnSurfaceChange(int surface_width, int surface_height) {
    LOGI("Opengl OnSurfaceChange in surface width:%d, height:%d", surface_width, surface_height);
    m_surface_width = surface_width;
    m_surface_height = surface_height;
    if (m_egl_thread != NULL) {
        m_egl_thread->OnSurfaceChange();
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
        delete m_base_opengl;
        m_base_opengl = NULL;
    }
    /**
     * 释放native surface
     * 避免内存泄漏
     */
    if (m_native_window_ != NULL) {
        ANativeWindow_release(m_native_window_);
        m_native_window_ = NULL;
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
        m_egl_thread->NotifyRender();
    }
}

void Opengl::SetImgData(int image_width, int image_height, int size, void* data) {
    m_image_width = image_width;
    m_image_height = image_height;
    if (m_image_pixels != NULL) {//释放上一张图片数据
        free(m_image_pixels);
        m_image_pixels = NULL;
    }
    m_image_pixels = malloc(size);
    memcpy(m_image_pixels, data, size);
    if (m_base_opengl != NULL) {
        /**
         * 这里有个小问题：就是如果gl渲染子线程还未来得及渲染，而SetImgData调用太快，那么新的图片数据会覆盖旧的图片数据，
         * 导致渲染出来的图片是最后一次调用SetImgData的图片数据，而不是上次调用SetImgData的图片数据
         * 当然一般情况下，这个问题不大，因为图片渲染的速度一般比较快。
         */
        m_base_opengl->SetImagePixel(image_width, image_height, m_image_pixels);
    }
    if (m_egl_thread != NULL) {
        m_egl_thread->NotifyRender();
    }
}

void Opengl::SetYuvData(int yuv_width, int yuv_height, void *y_data, void *u_data, void *v_data) {
    if (m_base_opengl != NULL) {
        m_base_opengl->SetYuvData(yuv_width, yuv_height, y_data, u_data, v_data);
    }
    if (m_egl_thread != NULL) {
        m_egl_thread->NotifyRender();
    }
}
