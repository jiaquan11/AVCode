package com.jiaquan.mymusic;

import android.Manifest;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.jiaquan.myplayer.demo.MyGLSurfaceViw;
import com.jiaquan.myplayer.demo.VideoDataPlayTest;

/*
SurfaceView和GLSurfaceView在处理Surface的方式上有一些不同。
SurfaceView提供一个独立的 Surface，这个Surface不会和主Surface（即应用的界面）共享上下文。
这意味着你可以在一个单独的线程中更新SurfaceView的Surface，而不会影响主界面的渲染。这对于视频播放和游戏等需要高性能渲染的场景非常有用。
另一方面，GLSurfaceView 是为了方便使用OpenGLES渲染而设计的。它内部管理了一个EGL上下文和一个GLSurfaceView.Renderer对象。
GLSurfaceView的Surface是用于OpenGLES渲染的，而不是用于MediaCodec的。
当你尝试将MediaCodec配置到GLSurfaceView的Surface上时,可能会遇到问题，因为这个Surface是为OpenGLES渲染准备的，而不是为MediaCodec准备的。
如果你想要将视频解码到Surface上，你需要创建一个SurfaceTexture，然后使用SurfaceTexture创建一个Surface。这个Surface可以用于MediaCodec的配置。

所以这里尝试的获取GLSurfaceView的surface,然后给到MediaCodec进行config,会崩溃。
*/
public class VideoTest3Activity extends AppCompatActivity {
    private static final String TAG = VideoTest3Activity.class.getSimpleName();

    private MyGLSurfaceViw myGLSurfaceViw = null;
    private VideoDataPlayTest videoDataPlayTest = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_videotest3);

        // 要申请的权限
        String[] permissions = {Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.ACCESS_NETWORK_STATE, Manifest.permission.CHANGE_NETWORK_STATE};
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions, 321);
        }

        videoDataPlayTest = new VideoDataPlayTest();

        myGLSurfaceViw = findViewById(R.id.myGlSurfaceView);
        myGLSurfaceViw.setOnSurfaceListener(new MyGLSurfaceViw.OnSurfaceListener() {
            @Override
            public void init(Surface surface) {
                Log.i(TAG, "VideoTest3Activity init surface: " + surface);
                videoDataPlayTest.setSurface(surface);
            }
        });
    }

    public void play(View view) {
//        videoPlayTest.start();
        videoDataPlayTest.start();
    }
}
