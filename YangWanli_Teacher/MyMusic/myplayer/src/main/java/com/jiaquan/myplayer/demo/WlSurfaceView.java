package com.jiaquan.myplayer.demo;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

//继承SurfaceView组件
public class WlSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
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
        void init(Surface surface);
    }
    public void setOnSurfaceListener(OnSurfaceListener onSurfaceListener) {
        this.onSurfaceListener = onSurfaceListener;
    }

    //SurfaceView创建
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (onSurfaceListener != null) {
            onSurfaceListener.init(holder.getSurface());
        }
    }

    //SurfaceView尺寸变化
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    //SurfaceView尺寸销毁
    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {//界面资源回收

    }
}
