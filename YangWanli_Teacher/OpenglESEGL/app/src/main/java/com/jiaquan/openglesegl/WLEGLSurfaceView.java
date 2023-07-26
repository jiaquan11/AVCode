package com.jiaquan.openglesegl;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.lang.ref.WeakReference;

import javax.microedition.khronos.egl.EGLContext;

/*
* 自定义一个GLSurfaceView的类
* 系统的GLSurfaceView的类也是通过继承SurfaceView，并创建EGL和渲染子线程等操作
* */
public abstract class WLEGLSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
    private Surface surface = null;
    private EGLContext eglContext = null;

    private WLEGLThread wleglThread = null;
    private WLGLRender wlglRender = null;

    private int mRenderMode = RENDERMODE_CONTINUOUSLY;

    public final static int RENDERMODE_WHEN_DIRTY = 0;
    public final static int RENDERMODE_CONTINUOUSLY = 1;

    public WLEGLSurfaceView(Context context) {
        this(context, null);
    }

    public WLEGLSurfaceView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public WLEGLSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        getHolder().addCallback(this);//注册回调的监听者
    }

    public void setRender(WLGLRender wlglRender) {
        this.wlglRender = wlglRender;
    }

    public void setRenderMode(int renderMode) {
        if (wlglRender == null) {
            throw new RuntimeException("must set render before");
        }
        this.mRenderMode = renderMode;
    }

    //设置外部的surface和共享EGL上下文
    public void setSurfaceAndEglContext(Surface surface, EGLContext eglContext) {
        this.surface = surface;
        this.eglContext = eglContext;
    }

    //获取当前GL线程的上下文
    public EGLContext getEglContext() {
        if (wleglThread != null) {
            return wleglThread.getEglContext();
        }
        return null;
    }

    //请求渲染
    public void requestRender() {
        if (wleglThread != null) {
            wleglThread.requestRender();
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (surface == null) {
            surface = holder.getSurface();//获取SurfaceView的Surface
        }

        wleglThread = new WLEGLThread(new WeakReference<WLEGLSurfaceView>(this));//创建一个子线程
        wleglThread.isCreate = true;
        wleglThread.start();//开启子线程
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        wleglThread.width = width;
        wleglThread.height = height;
        wleglThread.isChange = true;
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {//界面资源回收
        wleglThread.onDestroy();
        wleglThread = null;
        surface = null;
        eglContext = null;
    }

    public interface WLGLRender {
        void onSurfaceCreated();

        void onSurfaceChanged(int width, int height);

        void onDrawFrame();
    }

    static class WLEGLThread extends Thread {
        private WeakReference<WLEGLSurfaceView> wleglSurfaceViewWeakReference = null;
        private EglHelper eglHelper = null;
        private Object object = null;

        private boolean isExit = false;
        private boolean isCreate = false;
        private boolean isChange = false;
        private boolean isStart = false;

        private int width = 0;
        private int height = 0;

        public WLEGLThread(WeakReference<WLEGLSurfaceView> wleglSurfaceViewWeakReference) {
            this.wleglSurfaceViewWeakReference = wleglSurfaceViewWeakReference;
        }

        @Override
        public void run() {
            super.run();
            isExit = false;
            isStart = false;

            object = new Object();

            eglHelper = new EglHelper();
            eglHelper.initEgl(wleglSurfaceViewWeakReference.get().surface, wleglSurfaceViewWeakReference.get().eglContext);

            while (true) {
                if (isExit) {
                    release();//释放资源
                    break;
                }

                if (isStart) {
                    if (wleglSurfaceViewWeakReference.get().mRenderMode == RENDERMODE_WHEN_DIRTY) {
                        synchronized (object) {
                            try {
                                object.wait();
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    } else if (wleglSurfaceViewWeakReference.get().mRenderMode == RENDERMODE_CONTINUOUSLY) {
                        try {
                            Thread.sleep(1000 / 60);//1秒内持续刷60帧
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    } else {
                        throw new RuntimeException("mRenderMode is wrong value");
                    }
                }

                onCreate();
                onChange(width, height);
                onDraw();

                isStart = true;
            }
        }

        private void onCreate() {
            if (isCreate && wleglSurfaceViewWeakReference.get().wlglRender != null) {
                isCreate = false;
                wleglSurfaceViewWeakReference.get().wlglRender.onSurfaceCreated();
            }
        }

        private void onChange(int width, int height) {
            if (isChange && wleglSurfaceViewWeakReference.get().wlglRender != null) {
                isChange = false;
                wleglSurfaceViewWeakReference.get().wlglRender.onSurfaceChanged(width, height);
            }
        }

        private void onDraw() {
            if ((wleglSurfaceViewWeakReference.get().wlglRender != null) && (eglHelper != null)) {
                wleglSurfaceViewWeakReference.get().wlglRender.onDrawFrame();
//                if (!isStart) {
//                    wleglSurfaceViewWeakReference.get().wlglRender.onDrawFrame();
//                }
                eglHelper.swapBuffers();
            }
        }

        public void requestRender() {
            if (object != null) {
                synchronized (object) {
                    object.notifyAll();
                }
            }
        }

        public void onDestroy() {
            isExit = true;
            requestRender();
        }

        public EGLContext getEglContext() {
            if (eglHelper != null) {
                return eglHelper.getEglContext();
            }
            return null;
        }

        private void release() {
            if (eglHelper != null) {
                eglHelper.destroyEgl();
                eglHelper = null;
                object = null;
                wleglSurfaceViewWeakReference = null;
            }
        }
    }
}
