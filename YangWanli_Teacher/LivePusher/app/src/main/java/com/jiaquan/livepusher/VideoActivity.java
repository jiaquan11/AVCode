package com.jiaquan.livepusher;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.jiaquan.livepusher.camera.WLCameraView;
import com.jiaquan.livepusher.encodec.WLBaseMediaEncoder;
import com.jiaquan.livepusher.encodec.WLMediaEncoder;
import com.ywl5320.libmusic.WlMusic;
import com.ywl5320.listener.OnCompleteListener;
import com.ywl5320.listener.OnPreparedListener;
import com.ywl5320.listener.OnShowPcmDataListener;

/**
 * 摄像头预览及录制编码
 */
public class VideoActivity extends AppCompatActivity {
    private WLCameraView mWlCameraView_ = null;
    private Button mRecordBtn_ = null;
    private WLMediaEncoder mWlMediaEncoder_ = null;
    private WlMusic mWlMusic_ = null;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.avtivity_video);
        mWlCameraView_ = findViewById(R.id.cameraview);
        mRecordBtn_ = findViewById(R.id.btn_record);
        mWlMusic_ = WlMusic.getInstance();
        mWlMusic_.setCallBackPcmData(true);
        mWlMusic_.setOnPreparedListener(new OnPreparedListener() {
            @Override
            public void onPrepared() {
                mWlMusic_.playCutAudio(24, 44);
            }
        });

        mWlMusic_.setOnCompleteListener(new OnCompleteListener() {
            @Override
            public void onComplete() {
                mWlMediaEncoder_.stopRecord();
                mWlMediaEncoder_ = null;
                mWlMusic_.stop();
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mRecordBtn_.setText("开始录制");
                    }
                });
            }
        });

        mWlMusic_.setOnShowPcmDataListener(new OnShowPcmDataListener() {
            @Override
            public void onPcmInfo(int samplerate, int bit, int channels) {
                Log.i("LivePusherPlayer", "textureId is " + mWlCameraView_.getTextureId());
                mWlMediaEncoder_ = new WLMediaEncoder(VideoActivity.this);
                mWlMediaEncoder_.setTexture(mWlCameraView_.getTextureId(), mWlCameraView_.getFboWidth(), mWlCameraView_.getFboHeight());
                String destPath = "/sdcard/testziliao/yangwlVideo.mp4";
                mWlMediaEncoder_.initMediaEncoder(destPath, 720, 1280, samplerate, channels);
                mWlMediaEncoder_.setSurfaceAndEglContext(null, mWlCameraView_.getEglContext());
                mWlMediaEncoder_.setOnMediaInfoListener(new WLBaseMediaEncoder.OnMediaInfoListener() {
                    @Override
                    public void onMediaTime(int times) {
                        Log.i("LivePusherPlayer", "record time is: " + times);
                    }
                });
                mWlMediaEncoder_.startRecord();
            }

            @Override
            public void onPcmData(byte[] pcmdata, int size, long clock) {
                if (mWlMediaEncoder_ != null) {
                    mWlMediaEncoder_.putPCMData(pcmdata, size);
                }
            }
        });
    }

    public void record(View view) {
        if (mWlMediaEncoder_ == null) {//开始录制
            mWlMusic_.setSource("/sdcard/testziliao/yongqi-liangjingru.m4a");
            mWlMusic_.prePared();
            mRecordBtn_.setText("正在录制");
        } else {//停止录制
            mWlMediaEncoder_.stopRecord();
            mWlMediaEncoder_ = null;
            mWlMusic_.stop();
            mRecordBtn_.setText("开始录制");
        }
    }
}
