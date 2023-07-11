package com.jiaquan.jnithread;

public class ThreadDemo {
    //加载so
    static {
        System.loadLibrary("native-lib");
    }

    public interface OnErrorListener {
        void onError(int code, String msg);
    }
    private OnErrorListener onErrorListener = null;
    public void setOnErrorListener(OnErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }

    //called by native
    public void onError(int code, String msg) {
        if (onErrorListener != null) {
            onErrorListener.onError(code, msg);
        }
    }

    //native方法
    public native void normalThread();

    public native void mutexThread();

    public native void callBackFromC();
}
