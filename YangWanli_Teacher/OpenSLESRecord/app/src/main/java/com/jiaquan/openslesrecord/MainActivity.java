package com.jiaquan.openslesrecord;

import android.Manifest;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    //加载so
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // 要申请的权限
        String[] permissions = {Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE
                , Manifest.permission.ACCESS_NETWORK_STATE, Manifest.permission.CHANGE_NETWORK_STATE, Manifest.permission.CAMERA,
                Manifest.permission.RECORD_AUDIO};

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions, 321);
        }
    }

    //开始音频录制,采集的是音频pcm数据
    public void start(View view) {
        startRecord(Environment.getExternalStorageDirectory().getAbsolutePath() + "/testziliao/opensles_record.pcm");
    }

    //结束音频录制
    public void stop(View view) {
        stopRecord();
    }

    //native方法
    public native void startRecord(String path);

    public native void stopRecord();
}