package com.jiaquan.myplayer.demo;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class WlSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
    public WlSurfaceView(Context context) {
        this(context, null);
    }

    public WlSurfaceView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public WlSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        getHolder().addCallback(this);
    }

    private OnSurfaceListener mOnSurfaceListener_ = null;
    public interface OnSurfaceListener {
        void init(Surface surface);
    }
    public void setOnSurfaceListener(OnSurfaceListener onSurfaceListener) {
        mOnSurfaceListener_ = onSurfaceListener;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (mOnSurfaceListener_ != null) {
            mOnSurfaceListener_.init(holder.getSurface());
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }
}
