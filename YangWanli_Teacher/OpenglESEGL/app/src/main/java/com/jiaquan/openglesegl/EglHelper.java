package com.jiaquan.openglesegl;

import android.annotation.TargetApi;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.os.Build;
import android.util.Log;
import android.view.Surface;

@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
public class EglHelper {
    private static final String TAG = EglHelper.class.getSimpleName();
    private EGLDisplay mEglDisplay_ = null;
    private EGLContext mEglContext_ = null;
    private EGLSurface mEglSurface_ = null;

    public void initEgl(Surface surface, EGLContext eglContext) {
        mEglDisplay_ = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        if (mEglDisplay_ == EGL14.EGL_NO_DISPLAY) {
            throw new RuntimeException("eglGetDisplay failed!");
        }
        int[] version = new int[2];
        if (!EGL14.eglInitialize(mEglDisplay_, version, 0, version, 1)) {
            throw new RuntimeException("eglGetDisplay failed!");
        }
        int[] attributes = new int[] {
            EGL14.EGL_RED_SIZE, 8,
            EGL14.EGL_GREEN_SIZE, 8,
            EGL14.EGL_BLUE_SIZE, 8,
            EGL14.EGL_ALPHA_SIZE, 8,
            EGL14.EGL_DEPTH_SIZE, 8,
            EGL14.EGL_STENCIL_SIZE, 8,
            EGL14.EGL_RENDERABLE_TYPE, 4,
            EGL14.EGL_NONE
        };
        int[] num_config = new int[1];
        EGLConfig[] configs = new EGLConfig[1];
        if (!EGL14.eglChooseConfig(mEglDisplay_, attributes, 0, configs, 0, configs.length, num_config, 0)) {
            throw new IllegalArgumentException("eglChooseConfig failed!");
        }
        int numConfigs = num_config[0];
        if (numConfigs <= 0) {
            throw new IllegalArgumentException("No configs match configSpec!");
        }
        configs = new EGLConfig[numConfigs];
        if (!EGL14.eglChooseConfig(mEglDisplay_, attributes, 0, configs, 0, configs.length, num_config, 0)) {
            throw new IllegalArgumentException("eglChooseConfig2 failed!");
        }
        int[] attribList = {EGL14.EGL_CONTEXT_CLIENT_VERSION, 2, EGL14.EGL_NONE};
        Log.i(TAG, "eglContext: " + eglContext);
        if (eglContext != null) {
            mEglContext_ = EGL14.eglCreateContext(mEglDisplay_, configs[0], eglContext, attribList, 0);
        } else {
            mEglContext_ = EGL14.eglCreateContext(mEglDisplay_, configs[0], EGL14.EGL_NO_CONTEXT, attribList, 0);
        }
        int[] surfaceAttribs = {EGL14.EGL_NONE};
        mEglSurface_ = EGL14.eglCreateWindowSurface(mEglDisplay_, configs[0], surface, surfaceAttribs, 0);
        if (!EGL14.eglMakeCurrent(mEglDisplay_, mEglSurface_, mEglSurface_, mEglContext_)) {
            throw new RuntimeException("eglMakeCurrent failed!");
        }
    }

    public void swapBuffers() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            EGL14.eglSwapBuffers(mEglDisplay_, mEglSurface_);
        }
    }

    public EGLContext getEglContext() {
        return mEglContext_;
    }

    public void destroyEgl() {
        EGL14.eglMakeCurrent(mEglDisplay_, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT);
        EGL14.eglDestroySurface(mEglDisplay_, mEglSurface_);
        EGL14.eglDestroyContext(mEglDisplay_, mEglContext_);
        EGL14.eglTerminate(mEglDisplay_);
        mEglSurface_ = null;
        mEglContext_ = null;
        mEglDisplay_ = null;
    }
}
