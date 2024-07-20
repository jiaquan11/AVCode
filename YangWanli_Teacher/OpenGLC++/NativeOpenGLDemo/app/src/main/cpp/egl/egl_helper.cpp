#include "egl_helper.h"

EglHelper::EglHelper() {
    m_egl_display_ = EGL_NO_DISPLAY;
    m_egl_surface_ = EGL_NO_SURFACE;
    m_egl_config_ = NULL;
    m_egl_context_ = EGL_NO_CONTEXT;
}

EglHelper::~EglHelper() {

}

int EglHelper::InitEgl(EGLNativeWindowType window) {
    //1.得到默认的显示设备
    m_egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_egl_display_ == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay error");
        return -1;
    }

    //2.初始化默认的显示设备，可以获取得到设备的版本号
    EGLint *version = new EGLint[2];
    if (!eglInitialize(m_egl_display_, &version[0], &version[1])) {
        LOGE("eglInitialize error");
        return -2;
    }

    //3.查询并设置显示设备的属性，调用两次
    const EGLint attribs[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 8,
            EGL_STENCIL_SIZE, 8,//模板
            EGL_RENDERABLE_TYPE,
            EGL_OPENGL_ES2_BIT,//使用opengles2版本
            EGL_NONE
    };
    EGLint num_config;
    //查询最优config
    if (!eglChooseConfig(m_egl_display_, attribs, NULL, 1, &num_config)) {
        LOGE("eglChooseConfig first error");
        return -3;
    }

    //4 以上已查询到config,现直接保存到mEglConfig中
    if (!eglChooseConfig(m_egl_display_, attribs, &m_egl_config_, num_config, &num_config)) {
        LOGE("eglChooseConfig twice error");
        return -4;
    }

    //5.创建EGLContext
    int attrib_list[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    m_egl_context_ = eglCreateContext(m_egl_display_, m_egl_config_, EGL_NO_CONTEXT, attrib_list);
    if (m_egl_context_ == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext error");
        return -5;
    }

    //6.创建渲染的Surface   window:Android传递下来的SurfaceView的窗口，用于显示
    //而mEglSurface是后台缓存
    m_egl_surface_ = eglCreateWindowSurface(m_egl_display_, m_egl_config_, window, NULL);
    if (m_egl_surface_ == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface error");
        return -6;
    }

    //7.绑定EglContext和Surface到显示设备中,显示上下文和显示界面绑定到显示设备中
    if (!eglMakeCurrent(m_egl_display_, m_egl_surface_, m_egl_surface_, m_egl_context_)) {
        LOGE("eglMakeCurrent error");
        return -7;
    }
    LOGI("egl init success!");
    return 0;
}

/**
 * 交换前后台缓存
 */
int EglHelper::SwapBuffers() {
    if ((m_egl_display_ != EGL_NO_DISPLAY) && (m_egl_surface_ != EGL_NO_SURFACE)) {
        /**
         * 前台显示设备和后台缓存交换数据
         */
        if (!eglSwapBuffers(m_egl_display_, m_egl_surface_)) {
            LOGE("eglSwapBuffers error");
            return -1;
        }
    }
    return 0;
}

/**
 * 销毁EGL
 */
void EglHelper::DestroyEgl() {
    if (m_egl_display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(m_egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    if ((m_egl_display_ != EGL_NO_DISPLAY) && (m_egl_surface_ != EGL_NO_SURFACE)) {
        eglDestroySurface(m_egl_display_, m_egl_surface_);
        m_egl_surface_ = EGL_NO_SURFACE;
    }
    if ((m_egl_display_ != EGL_NO_DISPLAY) && (m_egl_context_ != EGL_NO_CONTEXT)) {
        eglDestroyContext(m_egl_display_, m_egl_context_);
        m_egl_context_ = EGL_NO_CONTEXT;
    }
    if (m_egl_display_ != EGL_NO_DISPLAY) {
        eglTerminate(m_egl_display_);
        m_egl_display_ = EGL_NO_DISPLAY;
    }
}
