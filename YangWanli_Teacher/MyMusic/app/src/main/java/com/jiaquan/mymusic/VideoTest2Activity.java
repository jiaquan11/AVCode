package com.jiaquan.mymusic;

import android.Manifest;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.jiaquan.myplayer.demo.VideoDataPlayTest;
import com.jiaquan.myplayer.demo.WlSurfaceView;

/**
 * 直接使用SurfaceView控件，将surface配置给硬件解码器，然后直接解码渲染出来
 * MediaCodec内部会进行解码操作，解码出来的数据会直接渲染到surface上
 */
public class VideoTest2Activity extends AppCompatActivity {
    private static final String TAG = VideoTest2Activity.class.getSimpleName();

    private WlSurfaceView mWLSurfaceView_ = null;
    private VideoDataPlayTest mVideoDataPlayTest_ = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_videotest2);
        // 要申请的权限
        String[] permissions = {Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.ACCESS_NETWORK_STATE, Manifest.permission.CHANGE_NETWORK_STATE};
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions, 321);
        }

        mVideoDataPlayTest_ = new VideoDataPlayTest(this);
        mWLSurfaceView_ = findViewById(R.id.wlsurfaceview);
        mWLSurfaceView_.setOnSurfaceListener(new WlSurfaceView.OnSurfaceListener() {
            @Override
            public void init(Surface surface) {
                Log.i(TAG, "VideoTest2Activity init surface: " + surface);
                mVideoDataPlayTest_.setSurface(surface);
            }
        });
    }

    public void play(View view) {
//        videoPlayTest.start();
        mVideoDataPlayTest_.start();
    }
}
