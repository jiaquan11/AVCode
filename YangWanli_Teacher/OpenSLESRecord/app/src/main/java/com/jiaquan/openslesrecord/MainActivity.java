package com.jiaquan.openslesrecord;

import android.Manifest;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
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

    /**
     * 开始音频录制
     */
    public void start(View view) {
        _nativeStartRecord(Environment.getExternalStorageDirectory().getAbsolutePath() + "/testziliao/opensles_record.pcm");
    }

    /**
     * 停止音频录制
     */
    public void stop(View view) {
        _nativeStopRecord();
    }

    private native void _nativeStartRecord(String path);

    private native void _nativeStopRecord();
}