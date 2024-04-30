package com.jiaquan.jnithread;

import android.os.Bundle;
import android.util.Log;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = MainActivity.class.getSimpleName();

    private ThreadDemo mThreadDemo_ = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mThreadDemo_ = new ThreadDemo();
        mThreadDemo_.setOnErrorListener(new ThreadDemo.OnErrorListener() {
            @Override
            public void onError(int code, String msg) {
                Log.e(TAG, "code is: " + code + " msg is: " + msg);
            }
        });
    }

    public void normal(View view) {
        mThreadDemo_.nativeNormalThread();
    }

    public void mutexThread(View view) {
        mThreadDemo_.nativeMutexThread();
    }

    public void callJavaMethod(View view) {
        mThreadDemo_.nativeCallBackFromC();
    }
}