package com.jiaquan.myplayer.demo;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MyGLSurfaceViw extends GLSurfaceView implements GLSurfaceView.Renderer{
    private static final String TAG = MyGLSurfaceViw.class.getSimpleName();
    private Surface surface = null;

    public MyGLSurfaceViw(Context context) {
        this(context, null);
    }

    public MyGLSurfaceViw(Context context, AttributeSet attrs) {
        super(context, attrs);

        setEGLContextClientVersion(2);//设置EGL上下文的版本号
        setRenderer(this);
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

//        surface = getHolder().getSurface();
        Log.i(TAG, "MyGLSurfaceViw construct surface: " + surface);
    }

    private OnSurfaceListener onSurfaceListener = null;
    public interface OnSurfaceListener {
        void init(Surface surface);
    }
    public void setOnSurfaceListener(OnSurfaceListener onSurfaceListener) {
        this.onSurfaceListener = onSurfaceListener;
    }


    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        if (surface == null) {
            surface = getHolder().getSurface();
            if (onSurfaceListener != null) {
                Log.i(TAG, "onSurfaceCreated surface is " + surface);
                onSurfaceListener.init(surface);
            }
        }
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Log.i(TAG, "onSurfaceChanged" );
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        Log.i(TAG, "onDrawFrame" );
    }
}
