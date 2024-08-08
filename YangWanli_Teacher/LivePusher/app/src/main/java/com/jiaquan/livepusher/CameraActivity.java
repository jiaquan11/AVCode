package com.jiaquan.livepusher;

import android.content.res.Configuration;
import android.os.Bundle;
import android.util.Log;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.jiaquan.livepusher.camera.WLCameraView;

/**
 * 摄像头预览
 */
public class CameraActivity extends AppCompatActivity {
    private WLCameraView mWlCameraView_ = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i("LivePusherPlayer", "CameraActivity onCreate");
        setContentView(R.layout.activity_camera);
        mWlCameraView_ = findViewById(R.id.cameraview);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.i("LivePusherPlayer", "CameraActivity onDestroy");
        mWlCameraView_.onDestroy();
    }

    /**
     * 当设备配置发生变化时，系统会调用这个方法
     * 配置变化通常包括屏幕方向的变化、键盘可用状态的变化、语言的变化等
     */
    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        Log.i("LivePusherPlayer", "onConfigurationChanged");
        mWlCameraView_.previewAngle(this);
    }

    /**
     * 切换摄像头
     */
    public void switchCamera(View view) {
        mWlCameraView_.switchCamera();
    }
}
