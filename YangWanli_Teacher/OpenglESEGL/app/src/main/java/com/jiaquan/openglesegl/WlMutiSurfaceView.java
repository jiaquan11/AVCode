package com.jiaquan.openglesegl;

import android.content.Context;
import android.util.AttributeSet;

public class WlMutiSurfaceView extends WLEGLSurfaceView{
    private WLMutiRender mWlMutiRender_ = null;

    public WlMutiSurfaceView(Context context) {
        this(context, null);
    }

    public WlMutiSurfaceView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public WlMutiSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        mWlMutiRender_ = new WLMutiRender(context);
        setRender(mWlMutiRender_);
        setRenderMode(RENDERMODE_WHEN_DIRTY);
    }

    public void setTextureId(int textureId, int index) {
        if (mWlMutiRender_ != null){
            mWlMutiRender_.setTextureId(textureId, index);
        }
    }
}
