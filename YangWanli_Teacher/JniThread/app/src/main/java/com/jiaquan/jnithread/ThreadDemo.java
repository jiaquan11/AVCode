package com.jiaquan.jnithread;

public class ThreadDemo {
    static {
        System.loadLibrary("native-lib");
    }

    public interface OnErrorListener {
        void onError(int code, String msg);
    }

    private OnErrorListener onErrorListener_ = null;

    public void setOnErrorListener(OnErrorListener onErrorListener) {
        this.onErrorListener_ = onErrorListener;
    }

    @CalledByNative
    public void onError(int code, String msg) {
        if (onErrorListener_ != null) {
            onErrorListener_.onError(code, msg);
        }
    }

    // native方法
    public native void nativeNormalThread();

    public native void nativeMutexThread();

    public native void nativeCallBackFromC();
}
