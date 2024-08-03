package com.jiaquan.openglesegl;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.lang.ref.WeakReference;

import javax.microedition.khronos.egl.EGLContext;

/**
 * 自定义一个GLSurfaceView的类
 * 系统的GLSurfaceView的类也是通过继承SurfaceView，
 * 并创建EGL和渲染子线程等操作
 */
public abstract class WLEGLSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
    public final static int RENDERMODE_WHEN_DIRTY = 0;
    public final static int RENDERMODE_CONTINUOUSLY = 1;
    private Surface mSurface_ = null;
    private EGLContext mEglContext_ = null;
    private WLEGLThread mWleglThread_ = null;
    private WLGLRender mWlglRender_ = null;
    private int mRenderMode_ = RENDERMODE_CONTINUOUSLY;

    public WLEGLSurfaceView(Context context) {
        this(context, null);
    }

    public WLEGLSurfaceView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public WLEGLSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        getHolder().addCallback(this);//设置SurfaceHolder的回调为当前类对象
    }

    public void setRender(WLGLRender wlglRender) {
        mWlglRender_ = wlglRender;
    }

    public void setRenderMode(int renderMode) {
        if (mWlglRender_ != null) {
            mRenderMode_ = renderMode;
        }
    }

    public void setSurfaceAndEglContext(Surface surface, EGLContext eglContext) {
        mSurface_ = surface;
        mEglContext_ = eglContext;
    }

    public EGLContext getEglContext() {
        if (mWleglThread_ != null) {
            return mWleglThread_.getEglContext();
        }
        return null;
    }

    public void requestRender() {
        if (mWleglThread_ != null) {
            mWleglThread_.requestRender();
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (mSurface_ == null) {
            mSurface_ = holder.getSurface();
        }
        /**
         * 创建一个模拟渲染线程，将WLEGLSurfaceView弱引用传入
         * 弱引用相关知识：
         * 在Android中使用WeakReference通常是为了防止某些对象（例如 Activity, View 等）因为被其他对象持有引用而导致内存泄漏。
         * 这是因为Activity和View 通常会随着用户操作（如旋转屏幕或导航）被频繁创建和销毁。
         * 为了避免这些对象被意外持有引用（强引用）从而导致内存泄漏，弱引用非常有帮助。
         * 这里的WLEGLThread是一个线程类，它持有了WLEGLSurfaceView的弱引用。通过使用WeakReference：
         * 优化内存管理:保证WLEGLSurfaceView可以在不再被强引用持有时被垃圾回收器回收。
         * 避免因为WLEGLThread持有强引用而导致WLEGLSurfaceView无法被回收。
         * 防止内存泄漏：可以有效避免内存泄漏的发生。
         */
        mWleglThread_ = new WLEGLThread(new WeakReference<WLEGLSurfaceView>(this));//创建一个模拟渲染线程
        mWleglThread_.mIsCreate = true;
        mWleglThread_.start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        mWleglThread_.mWidth = width;
        mWleglThread_.mHeight = height;
        mWleglThread_.mIsChange = true;
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        mWleglThread_.onDestroy();
        mWleglThread_ = null;
        mSurface_ = null;
        mEglContext_ = null;
    }

    public interface WLGLRender {
        void onSurfaceCreated();
        void onSurfaceChanged(int width, int height);
        void onDrawFrame();
    }

    static class WLEGLThread extends Thread {
        private WeakReference<WLEGLSurfaceView> mWleglSurfaceViewWeakReference_;
        private EglHelper mEglHelper_ = null;
        private Object mObject_ = null;
        private boolean mIsExit_ = false;
        public  boolean mIsCreate = false;
        public boolean mIsChange = false;
        private boolean mIsStart_ = false;
        public int mWidth = 0;
        public int mHeight = 0;

        public WLEGLThread(WeakReference<WLEGLSurfaceView> wleglSurfaceViewWeakReference) {
            mWleglSurfaceViewWeakReference_ = wleglSurfaceViewWeakReference;
        }

        @Override
        public void run() {
            super.run();
            mIsExit_ = false;
            mIsStart_ = false;
            mObject_ = new Object();
            mEglHelper_ = new EglHelper();
            mEglHelper_.initEgl(mWleglSurfaceViewWeakReference_.get().mSurface_, mWleglSurfaceViewWeakReference_.get().mEglContext_);
            while (true) {
                if (mIsExit_) {
                    _release();
                    break;
                }

                if (mIsStart_) {
                    if (mWleglSurfaceViewWeakReference_.get().mRenderMode_ == RENDERMODE_WHEN_DIRTY) {
                        synchronized (mObject_) {
                            try {
                                mObject_.wait();
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    } else if (mWleglSurfaceViewWeakReference_.get().mRenderMode_ == RENDERMODE_CONTINUOUSLY) {
                        try {
                            Thread.sleep(1000 / 60);//1秒内持续刷60帧
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    } else {
                        throw new RuntimeException("mRenderMode is wrong value");
                    }
                }
                _onCreate();
                _onChange(mWidth, mHeight);
                _onDraw();
                mIsStart_ = true;
            }
        }

        private void _onCreate() {
            if (mIsCreate && mWleglSurfaceViewWeakReference_.get().mWlglRender_ != null) {
                mIsCreate = false;
                mWleglSurfaceViewWeakReference_.get().mWlglRender_.onSurfaceCreated();
            }
        }

        private void _onChange(int width, int height) {
            if (mIsChange && mWleglSurfaceViewWeakReference_.get().mWlglRender_ != null) {
                mIsChange = false;
                mWleglSurfaceViewWeakReference_.get().mWlglRender_.onSurfaceChanged(width, height);
            }
        }

        private void _onDraw() {
            if (mWleglSurfaceViewWeakReference_.get().mWlglRender_ != null) {
                mWleglSurfaceViewWeakReference_.get().mWlglRender_.onDrawFrame();
                mEglHelper_.swapBuffers();
            }
        }

        public void requestRender() {
            if (mObject_ != null) {
                synchronized (mObject_) {
                    mObject_.notifyAll();
                }
            }
        }

        public void onDestroy() {
            mIsExit_ = true;
            requestRender();
        }

        public EGLContext getEglContext() {
            if (mEglHelper_ != null) {
                return mEglHelper_.getEglContext();
            }
            return null;
        }

        private void _release() {
            if (mEglHelper_ != null) {
                mEglHelper_.destroyEgl();
                mEglHelper_ = null;
                mObject_ = null;
                mWleglSurfaceViewWeakReference_ = null;
            }
        }
    }
}
