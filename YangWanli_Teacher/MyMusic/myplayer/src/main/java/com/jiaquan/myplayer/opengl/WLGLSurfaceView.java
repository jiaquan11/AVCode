package com.jiaquan.myplayer.opengl;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

import com.jiaquan.myplayer.log.MyLog;

/*
这里使用GLSurfaceView控件, 用于视频渲染
GLSurfaceView控件底层已经内部封装好了EGL环境，不需要额外
配置EGL环境，只需要在对应的渲染器中进行绘制即可
 */
public class WLGLSurfaceView extends GLSurfaceView {
    private WLRender wlRender = null;

    public WLGLSurfaceView(Context context) {
        this(context, null);
    }

    public WLGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        MyLog.i("WLGLSurfaceView construct in");
        setEGLContextClientVersion(2);//设置EGL上下文的版本号
        wlRender = new WLRender(context);//渲染器实例
        setRenderer(wlRender);//设置渲染器，GLSurfaceView控件需要一个绘制的Renderer类
        //设置渲染模式->手动渲染,表示可以在收到数据时，主动请求控件调用onDrawFrame方法进行渲染，若是RENDERMODE_CONTINUOUSLY方式，会持续绘制，耗费性能
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

        //用于硬解
        wlRender.setOnRenderListener(new WLRender.OnRenderListener() {
            @Override
            public void onRender() {
                requestRender();//主动请求渲染一帧图像数据
            }
        });
        MyLog.i("WLGLSurfaceView construct end");
    }

    //设置YUV图像数据
    public void setYUVData(int width, int height, byte[] y, byte[] u, byte[] v) {
        if (wlRender != null) {
            wlRender.setYUVRenderData(width, height, y, u, v);
            requestRender();//主动请求渲染一帧图像数据
        }
    }

    //获取GLSurfaceView的渲染器
    public WLRender getWlRender() {
        return wlRender;
    }
}
