package com.jiaquan.mymusic;

import android.Manifest;
import android.os.Build;
import android.os.Bundle;
import android.view.Surface;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.jiaquan.myplayer.demo.VideoDataPlayTest;
import com.jiaquan.myplayer.demo.VideoPlayTest;
import com.jiaquan.myplayer.opengl.WLGLSurfaceView;
import com.jiaquan.myplayer.opengl.WLRender;

/**
 * 这个页面仅仅用于测试，测试视频播放渲染相关操作
 * 这里使用的是GLSurfaceView控件，使用GLSurfaceView的render线程，同时
 * 使用opengl创建一个OES纹理，并生成surface,配置给硬件解码器，然后更新surface使用openg进行绘制,
 * GLSurfaceView内部封装的EGL环境会自动进行渲染出来
 * 下面操作用例包括两个：
 * 1.使用MediaExtrator提取视频文件码流，丢给解码器
 * 2.使用视频裸流文件，直接读取并丢给解码器
 */
public class VideoTestActivity extends AppCompatActivity {
    private WLGLSurfaceView mWLglSurfaceView_ = null;
    private VideoPlayTest mVideoPlayTest_ = null;
    private VideoDataPlayTest mVideoDataPlayTest_ = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_videotest);

        // 要申请的权限
        String[] permissions = {Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.ACCESS_NETWORK_STATE, Manifest.permission.CHANGE_NETWORK_STATE};
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions, 321);
        }

//        mVideoPlayTest_ = new VideoPlayTest();
        mVideoDataPlayTest_ = new VideoDataPlayTest(this);

        mWLglSurfaceView_ = findViewById(R.id.WlglSurfaceView);
        mWLglSurfaceView_.getWlRender().setOnSurfaceCreateListener(new WLRender.OnSurfaceCreateListener() {
            @Override
            public void onSurfaceCreate(Surface surface) {
//                mVideoPlayTest_.setSurface(surface);
                mVideoDataPlayTest_.setSurface(surface);
            }
        });

        mWLglSurfaceView_.getWlRender().setRenderType(WLRender.RENDER_MEDIACODEC);
//        mWLglSurfaceView_.getWlRender().setVideoSize(mVideoPlayTest_.getWidth(), mVideoPlayTest_.getHeight());
        mWLglSurfaceView_.getWlRender().setVideoSize(mVideoDataPlayTest_.getWidth(), mVideoDataPlayTest_.getHeight());
    }

    public void play(View view) {
//        mVideoPlayTest_.start();
        mVideoDataPlayTest_.start();
    }
}
