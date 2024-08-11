package com.jiaquan.livepusher.egl;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.lang.ref.WeakReference;

import javax.microedition.khronos.egl.EGLContext;

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
        getHolder().addCallback(this);
    }

    public void setRender(WLGLRender wlglRender) {
        mWlglRender_ = wlglRender;
    }

    public void setRenderMode(int renderMode) {
        if (mWlglRender_ == null) {
            throw new RuntimeException("must set render before");
        }
        mRenderMode_ = renderMode;
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
        mWleglThread_ = new WLEGLThread(new WeakReference<WLEGLSurfaceView>(this));
        mWleglThread_.mIsCreate_ = true;
        mWleglThread_.start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        mWleglThread_.mWidth_ = width;
        mWleglThread_.mHeight_ = height;
        mWleglThread_.mIsChange_ = true;
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

        void onSurfaceDestroy();
    }

    static class WLEGLThread extends Thread {
        private WeakReference<WLEGLSurfaceView> mWleglSurfaceViewWeakReference_;
        private EglHelper mEglHelper_ = null;
        private Object mObject_ = null;
        private boolean mIsExit_ = false;
        private boolean mIsCreate_ = false;
        private boolean mIsChange_ = false;
        private boolean mIsStart_ = false;
        private int mWidth_;
        private int mHeight_;

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
                    release();
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
                            Thread.sleep(1000 / 60);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    } else {
                        throw new RuntimeException("mRenderMode is wrong value");
                    }
                }
                onCreate();
                onChange(mWidth_, mHeight_);
                onDraw();
                mIsStart_ = true;
            }
        }

        private void onCreate() {
            if (mIsCreate_ && mWleglSurfaceViewWeakReference_.get().mWlglRender_ != null) {
                mIsCreate_ = false;
                mWleglSurfaceViewWeakReference_.get().mWlglRender_.onSurfaceCreated();
            }
        }

        private void onChange(int width, int height) {
            if (mIsChange_ && mWleglSurfaceViewWeakReference_.get().mWlglRender_ != null) {
                mIsChange_ = false;
                mWleglSurfaceViewWeakReference_.get().mWlglRender_.onSurfaceChanged(width, height);
            }
        }

        private void onDraw() {
            if ((mWleglSurfaceViewWeakReference_.get().mWlglRender_ != null) && (mEglHelper_ != null) && mIsStart_) {
                mWleglSurfaceViewWeakReference_.get().mWlglRender_.onDrawFrame();
                mEglHelper_.swapBuffers();
            }
        }

        public void onDestroy() {
            mIsExit_ = true;
            requestRender();
        }

        private void requestRender() {
            if (mObject_ != null) {
                synchronized (mObject_) {
                    mObject_.notifyAll();
                }
            }
        }

        public EGLContext getEglContext() {
            if (mEglHelper_ != null) {
                return mEglHelper_.getEglContext();
            }
            return null;
        }

        public void release() {
            if (mWleglSurfaceViewWeakReference_.get().mWlglRender_ != null) {
                mWleglSurfaceViewWeakReference_.get().mWlglRender_.onSurfaceDestroy();
            }
            if (mEglHelper_ != null) {
                mEglHelper_.destroyEgl();
                mEglHelper_ = null;
                mObject_ = null;
            }
            mWleglSurfaceViewWeakReference_ = null;
        }
    }
}
