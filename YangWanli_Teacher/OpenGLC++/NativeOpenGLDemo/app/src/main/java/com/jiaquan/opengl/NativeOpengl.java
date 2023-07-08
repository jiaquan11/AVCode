package com.jiaquan.opengl;

import android.view.Surface;

public class NativeOpengl {
    static {
        System.loadLibrary("native-lib");
    }

    //native方法
    public native void surfaceCreate(Surface surface);

    public native void surfaceChange(int width, int height);

    public native void surfaceChangeFilter();

    public native void imgData(int w, int h, int length, byte[] data);

    public native void setYuvData(byte[] y, byte[] u, byte[] v, int w, int h);

    public native void surfaceDestroy();
}
