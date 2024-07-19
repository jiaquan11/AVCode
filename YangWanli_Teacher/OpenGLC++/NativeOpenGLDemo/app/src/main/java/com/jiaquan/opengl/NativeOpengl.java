package com.jiaquan.opengl;

import android.view.Surface;

public class NativeOpengl {
    static {
        System.loadLibrary("native-lib");
    }

    //native方法
    public native void nativeSurfaceCreate(Surface surface);

    public native void nativeSurfaceChange(int width, int height);

    public native void nativeSurfaceDestroy();

    public native void nativeSurfaceChangeFilter();

    public native void nativeSetImgData(int width, int height, int size, byte[] imageData);

    public native void nativeSetYuvData(int width, int height, byte[] yData, byte[] uData, byte[] vData);


}
