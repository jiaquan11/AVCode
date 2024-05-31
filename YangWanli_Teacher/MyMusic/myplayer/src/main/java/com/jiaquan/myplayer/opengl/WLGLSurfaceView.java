package com.jiaquan.myplayer.opengl;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

import com.jiaquan.myplayer.log.MyLog;

/**
 * GLSurfaceView控件, 用于视频渲染
 * GLSurfaceView控件底层已经内部封装好了EGL环境，不需要额外
 * 配置EGL环境，只需要在对应的渲染器中进行显示即可
 */
public class WLGLSurfaceView extends GLSurfaceView {
    private WLRender m_WlRender_ = null;

    public WLGLSurfaceView(Context context) {
        this(context, null);
    }

    public WLGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        MyLog.i("WLGLSurfaceView construct in");
        setEGLContextClientVersion(2);//设置EGL上下文的版本号
        m_WlRender_ = new WLRender(context);//渲染器实例
        setRenderer(m_WlRender_);//设置渲染器，GLSurfaceView控件需要一个绘制的Renderer类:独立的渲染线程
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

        //用于硬解，设置监听，当硬解完一帧收到数据就请求GLSurfaceView渲染
        m_WlRender_.setOnRenderListener(new WLRender.OnRenderListener() {
            @Override
            public void onRender() {
                requestRender();//主动请求渲染一帧图像数据
            }
        });
        MyLog.i("WLGLSurfaceView construct end");
    }

    //设置YUV图像数据
    public void setYUVData(int width, int height, byte[] y, byte[] u, byte[] v) {
        if (m_WlRender_ != null) {
            m_WlRender_.setYUVRenderData(width, height, y, u, v);
            requestRender();//主动请求渲染一帧图像数据
        }
    }

    //获取GLSurfaceView的渲染器
    public WLRender getWlRender() {
        return m_WlRender_;
    }
}
