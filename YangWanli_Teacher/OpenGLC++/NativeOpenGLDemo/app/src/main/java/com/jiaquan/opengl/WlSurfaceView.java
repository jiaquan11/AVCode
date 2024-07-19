package com.jiaquan.opengl;

import android.content.Context;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class WlSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
    private NativeOpengl mNativeOpengl_ = null;

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
        void init();
    }

    public void setOnSurfaceListener(OnSurfaceListener onSurfaceListener) {
        mOnSurfaceListener_ = onSurfaceListener;
    }

    public void setNativeOpengl(NativeOpengl nativeOpengl) {
        mNativeOpengl_ = nativeOpengl;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (mNativeOpengl_ != null) {
            mNativeOpengl_.nativeSurfaceCreate(holder.getSurface());
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (mNativeOpengl_ != null) {
            mNativeOpengl_.nativeSurfaceChange(width, height);
            //底层EGL环境已经全部搭建好，通知上层进行渲染
            if (mOnSurfaceListener_ != null) {
                mOnSurfaceListener_.init();
            }
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        if (mNativeOpengl_ != null) {
            mNativeOpengl_.nativeSurfaceDestroy();
        }
    }
}
