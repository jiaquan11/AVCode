package com.jiaquan.mymusic;

import android.Manifest;
import android.os.Build;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.jiaquan.myplayer.demo.H265VideoPlayTest;

/*
* 这个页面仅仅用于测试，测试视频播放渲染相关操作
* */
public class VideoTestActivity extends AppCompatActivity {
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_videotest);

        // 要申请的权限
        String[] permissions = {Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.ACCESS_NETWORK_STATE, Manifest.permission.CHANGE_NETWORK_STATE};
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions, 321);
        }
    }

    public void test(View view) {
        H265VideoPlayTest h265VideoPlayTest = new H265VideoPlayTest();
        h265VideoPlayTest.start();
    }
}
