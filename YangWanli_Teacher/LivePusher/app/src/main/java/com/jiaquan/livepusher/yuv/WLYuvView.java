package com.jiaquan.livepusher.yuv;

import android.content.Context;
import android.util.AttributeSet;

import com.jiaquan.livepusher.egl.WLEGLSurfaceView;

public class WLYuvView extends WLEGLSurfaceView {
    private WLYuvRender mWlYuvRender_ = null;

    public WLYuvView(Context context) {
        this(context, null);
    }

    public WLYuvView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public WLYuvView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        mWlYuvRender_ = new WLYuvRender(context);
        setRender(mWlYuvRender_);
        setRenderMode(WLEGLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    public void setYuvData(int yuvWidth, int yuvHeight, byte[] ydata, byte[] udata, byte[] vdata) {
        if (mWlYuvRender_ != null) {
            mWlYuvRender_.setYuvData(yuvWidth, yuvHeight, ydata, udata, vdata);
            requestRender();
        }
    }
}
