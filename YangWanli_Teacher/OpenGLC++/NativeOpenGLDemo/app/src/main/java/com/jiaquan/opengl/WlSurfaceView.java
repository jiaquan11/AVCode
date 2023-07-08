package com.jiaquan.opengl;

import android.content.Context;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

//继承SurfaceView组件
public class WlSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
    private NativeOpengl nativeOpengl = null;

    public WlSurfaceView(Context context) {
        this(context, null);
    }

    public WlSurfaceView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public WlSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        getHolder().addCallback(this);//当前组件监听回调函数
    }

    private OnSurfaceListener onSurfaceListener = null;

    public interface OnSurfaceListener {
        void init();
    }

    public void setOnSurfaceListener(OnSurfaceListener onSurfaceListener) {
        this.onSurfaceListener = onSurfaceListener;
    }

    public void setNativeOpengl(NativeOpengl nativeOpengl) {
        this.nativeOpengl = nativeOpengl;
    }

    //SurfaceView创建
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (nativeOpengl != null) {
            nativeOpengl.surfaceCreate(holder.getSurface());//传递surface到底层

//            if (onSurfaceListener != null) {
//                onSurfaceListener.init();
//            }
        }
    }

    //SurfaceView尺寸变化
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (nativeOpengl != null) {
            nativeOpengl.surfaceChange(width, height);//屏幕宽高

            //底层EGL环境已经全部搭建好，通知上层进行渲染
            if (onSurfaceListener != null) {
                onSurfaceListener.init();
            }
        }
    }

    //SurfaceView尺寸销毁
    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {//界面资源回收
        if (nativeOpengl != null) {
            nativeOpengl.surfaceDestroy();
        }
    }
}
