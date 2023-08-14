package com.jiaquan.openglesegl;

import android.content.Context;
import android.util.AttributeSet;

public class WLGLTextureView extends WLEGLSurfaceView {
    private WLTextureRender wlTextureRender = null;

    public WLGLTextureView(Context context) {
        this(context, null);
    }

    public WLGLTextureView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public WLGLTextureView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        wlTextureRender = new WLTextureRender(context);
        setRender(wlTextureRender);//设置渲染器
    }

    //获取渲染器
    public WLTextureRender getWlTextureRender() {
        return wlTextureRender;
    }
}
