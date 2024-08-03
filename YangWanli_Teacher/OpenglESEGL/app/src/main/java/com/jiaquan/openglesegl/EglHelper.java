package com.jiaquan.openglesegl;

import android.opengl.EGL14;
import android.util.Log;
import android.view.Surface;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

public class EglHelper {
    private static final String TAG = EglHelper.class.getSimpleName();

    private EGL10 mEgl_ = null;
    private EGLDisplay mEglDisplay_ = null;
    private EGLContext mEglContext_ = null;
    private EGLSurface mEglSurface_ = null;

    public void initEgl(Surface surface, EGLContext eglContext) {
        //1.得到Egl实例
        mEgl_ = (EGL10) EGLContext.getEGL();
        //2.得到默认的显示设备(就是窗口)
        mEglDisplay_ = mEgl_.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
        if (mEglDisplay_ == EGL10.EGL_NO_DISPLAY) {
            throw new RuntimeException("eglGetDisplay failed!");
        }
        //3.初始化默认显示设备
        int[] version = new int[2];
        if (!mEgl_.eglInitialize(mEglDisplay_, version)) {
            throw new RuntimeException("eglGetDisplay failed!");
        }
        //4.设置显示设备的属性
        int[] attributes = new int[] {
            EGL10.EGL_RED_SIZE, 8,
            EGL10.EGL_GREEN_SIZE, 8,
            EGL10.EGL_BLUE_SIZE, 8,
            EGL10.EGL_ALPHA_SIZE, 8,
            EGL10.EGL_DEPTH_SIZE, 8,
            EGL10.EGL_STENCIL_SIZE, 8,
            EGL10.EGL_RENDERABLE_TYPE, 4,
            EGL10.EGL_NONE
        };
        int[] num_config = new int[1];
        if (!mEgl_.eglChooseConfig(mEglDisplay_, attributes, null, 1, num_config)) {
            throw new IllegalArgumentException("eglChooseConfig failed!");
        }
        int numConfigs = num_config[0];
        if (numConfigs <= 0) {
            throw new IllegalArgumentException("No configs match configSpec!");
        }
        //5.从系统中获取对应属性的配置
        EGLConfig[] configs = new EGLConfig[numConfigs];
        if (!mEgl_.eglChooseConfig(mEglDisplay_, attributes, configs, numConfigs, num_config)) {
            throw new IllegalArgumentException("eglChooseConfig2 failed!");
        }
        //6.创建EglContext
        int[] attribList = {EGL14.EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE};
        Log.i(TAG, "eglContext: " + eglContext);
        if (eglContext != null) {
            mEglContext_ = mEgl_.eglCreateContext(mEglDisplay_, configs[0], eglContext, attribList);
        } else {
            mEglContext_ = mEgl_.eglCreateContext(mEglDisplay_, configs[0], EGL10.EGL_NO_CONTEXT, attribList);
        }
        //7.创建渲染的surface
        mEglSurface_ = mEgl_.eglCreateWindowSurface(mEglDisplay_, configs[0], surface, null);
        //8.绑定EglContext和Surface到显示设备中
        if (!mEgl_.eglMakeCurrent(mEglDisplay_, mEglSurface_, mEglSurface_, mEglContext_)) {
            throw new RuntimeException("eglMakeCurrent failed!");
        }
    }

    public boolean swapBuffers() {
        if (mEgl_ != null) {
            //9.刷新数据，显示渲染场景
            return mEgl_.eglSwapBuffers(mEglDisplay_, mEglSurface_);
        } else {
            throw new RuntimeException("egl is null");
        }
    }

    public EGLContext getEglContext() {
        return mEglContext_;
    }

    public void destroyEgl() {
        if (mEgl_ != null) {
            mEgl_.eglMakeCurrent(mEglDisplay_, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
            mEgl_.eglDestroySurface(mEglDisplay_, mEglSurface_);
            mEglSurface_ = null;
            mEgl_.eglDestroyContext(mEglDisplay_, mEglContext_);
            mEglContext_ = null;
            mEgl_.eglTerminate(mEglDisplay_);
            mEglDisplay_ = null;
            mEgl_ = null;
        }
    }
}
