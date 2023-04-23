package com.jiaquan.xplay;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.View;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

//这个类为GLSurfaceView组件，位于MainActivity页面中
public class XPlay extends GLSurfaceView implements SurfaceHolder.Callback, GLSurfaceView.Renderer, View.OnClickListener {
    public XPlay(Context context, AttributeSet attrs) {
        super(context, attrs);

        //android 8.0需要设置
        setRenderer(this);//当前页面为渲染器
        setOnClickListener(this);//GLSurfaceView组件可监听点击操作，用于暂停或播放
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.i("XPlay", "surfaceCreated");
        InitView(holder.getSurface());//设置surface到native
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {

    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int i, int i1) {

    }

    @Override
    public void onDrawFrame(GL10 gl10) {

    }

    @Override
    public void onClick(View view) {
        PlayOrPause();
    }

    //native method
    public native void InitView(Object surface);

    public native void PlayOrPause();
}
