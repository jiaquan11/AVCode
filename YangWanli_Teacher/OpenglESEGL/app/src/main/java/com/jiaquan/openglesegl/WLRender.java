package com.jiaquan.openglesegl;

import android.opengl.GLES20;

/**
 *自定义GLSurfaceView的渲染器类
 */
public class WLRender implements WLEGLSurfaceView.WLGLRender {
    public WLRender() {

    }

    @Override
    public void onSurfaceCreated() {

    }

    @Override
    public void onSurfaceChanged(int width, int height) {
        GLES20.glViewport(0, 0, width, height);
    }

    @Override
    public void onDrawFrame() {
        GLES20.glClearColor(1.0f, 0.0f, 0.0f, 1.0f);//先设置清屏颜色
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);//然后清屏，将整个窗口清屏，这样才能看到背景颜色
    }
}
