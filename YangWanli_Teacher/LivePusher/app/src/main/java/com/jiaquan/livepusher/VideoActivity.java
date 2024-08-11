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
    private WLCameraView wlCameraView = null;
    private Button btnRecord = null;
    private WLMediaEncoder wlMediaEncoder = null;
    private WlMusic wlMusic = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.avtivity_video);
        wlCameraView = findViewById(R.id.cameraview);
        btnRecord = findViewById(R.id.btn_record);
        wlMusic = WlMusic.getInstance();
        wlMusic.setCallBackPcmData(true);
        wlMusic.setOnPreparedListener(new OnPreparedListener() {
            @Override
            public void onPrepared() {
                wlMusic.playCutAudio(24, 44);
            }
        });

        wlMusic.setOnCompleteListener(new OnCompleteListener() {
            @Override
            public void onComplete() {
                wlMediaEncoder.stopRecord();
                wlMediaEncoder = null;
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        btnRecord.setText("开始录制");
                    }
                });
            }
        });

        wlMusic.setOnShowPcmDataListener(new OnShowPcmDataListener() {
            @Override
            public void onPcmInfo(int samplerate, int bit, int channels) {
                Log.i("VideoActivity", "textureId is " + wlCameraView.getTextureId());//回调回来的FBO最终渲染的窗口纹理id
                wlMediaEncoder = new WLMediaEncoder(VideoActivity.this);//使用摄像头预览最终渲染到窗口的纹理进行编码
                wlMediaEncoder.setTexture(wlCameraView.getTextureId(), wlCameraView.getFboWidth(), wlCameraView.getFboHeight());
                String destPath = "/sdcard/testziliao/yangwlVideo.mp4";
                wlMediaEncoder.initEncoder(destPath, 720, 1280, samplerate, channels);
                wlMediaEncoder.setSurfaceAndEglContext(null, wlCameraView.getEglContext());
                wlMediaEncoder.setOnMediaInfoListener(new WLBaseMediaEncoder.OnMediaInfoListener() {
                    @Override
                    public void onMediaTime(int times) {
                        Log.i("VideoActivity", "rec time is: " + times);
                    }
                });
                wlMediaEncoder.startRecord();
            }

            @Override
            public void onPcmData(byte[] pcmdata, int size, long clock) {
                if (wlMediaEncoder != null) {
                    wlMediaEncoder.putPCMData(pcmdata, size);
                }
            }
        });
    }

    public void record(View view) {
        if (wlMediaEncoder == null) {
            wlMusic.setSource("/sdcard/testziliao/yongqi-liangjingru.m4a");
            wlMusic.prePared();

            btnRecord.setText("正在录制");
        } else {//中途主动停止录制
            wlMediaEncoder.stopRecord();
            btnRecord.setText("开始录制");

            wlMediaEncoder = null;
            wlMusic.stop();
        }
    }
}
