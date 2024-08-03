package com.jiaquan.openglesegl;

import android.content.Context;
import android.util.AttributeSet;

public class WLGLTextureView extends WLEGLSurfaceView {
    private WLTextureRender mWlTextureRender_;

    public WLGLTextureView(Context context) {
        this(context, null);
    }

    public WLGLTextureView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public WLGLTextureView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        mWlTextureRender_ = new WLTextureRender(context);
        setRender(mWlTextureRender_);
        setRenderMode(WLEGLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    public WLTextureRender getWlTextureRender() {
        return mWlTextureRender_;
    }
}
