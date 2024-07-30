package com.jiaquan.opengl;

import android.view.Surface;

import com.jiaquan.utils.CalledByNative;

public class NativeOpengl {
    static {
        System.loadLibrary("native-lib");
    }

    private OnSurfaceListener mOnSurfaceListener_ = null;
    public interface OnSurfaceListener {
        void init();
    }

    public void setOnSurfaceListener(OnSurfaceListener onSurfaceListener) {
        mOnSurfaceListener_ = onSurfaceListener;
    }

    /**
     * EGL环境已创建好,通知上层进行渲染
     */
    @CalledByNative
    private void onCallEglPrepared() {
        if (mOnSurfaceListener_ != null) {
            mOnSurfaceListener_.init();
        }
    }

    //native方法
    public native void nativeSurfaceCreate(Surface surface);

    public native void nativeSurfaceChange(int surfaceWidth, int surfaceHeight);

    public native void nativeSurfaceDestroy();

    public native void nativeSurfaceChangeFilter();

    public native void nativeSetImgData(int imageWidth, int imageHeight, int size, byte[] imageData);

    public native void nativeSetYuvData(int yuvWidth, int yuvHeight, byte[] yData, byte[] uData, byte[] vData);
}
