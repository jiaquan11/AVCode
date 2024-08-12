package com.jiaquan.livepusher;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.jiaquan.livepusher.encodec.WLMediaEncoder;
import com.jiaquan.livepusher.imgvideo.WLImgVideoView;
import com.ywl5320.libmusic.WlMusic;
import com.ywl5320.listener.OnPreparedListener;
import com.ywl5320.listener.OnShowPcmDataListener;

public class ImageVideoActivity extends AppCompatActivity {
    private WLImgVideoView mWlImgVideoView_ = null;
    private WLMediaEncoder mWlMediaEncoder_ = null;
    private Button mMergeBtn_ = null;
    private WlMusic mWlMusic_ = null;
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_imagevideo);
        mWlImgVideoView_ = findViewById(R.id.imgvideoview);
        mMergeBtn_ = findViewById(R.id.button_merge_video);
        mWlMusic_ = WlMusic.getInstance();
        mWlMusic_.setCallBackPcmData(true);
        mWlMusic_.setOnPreparedListener(new OnPreparedListener() {
            @Override
            public void onPrepared() {
                Log.i("LivePusherPlayer", "onPrepared");
                mWlMusic_.playCutAudio(0, 60);
                //开始设置图片资源
                _startImgs();

            }
        });
        mWlMusic_.setOnShowPcmDataListener(new OnShowPcmDataListener() {
            @Override
            public void onPcmInfo(int sampleRate, int bitsPerSample, int channels) {
                Log.i("LivePusherPlayer", "onPcmInfo sampleRate: " + sampleRate + ", bitsPerSample: " + bitsPerSample + ", channels: " + channels);
                mWlMediaEncoder_ = new WLMediaEncoder(ImageVideoActivity.this);
                int fBOImageWidth = mWlImgVideoView_.getFboWidth();
                int fBOImageHeight = mWlImgVideoView_.getFboHeight();
                int fBOTextureId = mWlImgVideoView_.getFboTextureId();
                mWlMediaEncoder_.setTexture(mWlImgVideoView_.getFboTextureId(), fBOImageWidth, fBOImageHeight);
                mWlMediaEncoder_.setSurfaceAndEglContext(null, mWlImgVideoView_.getEglContext());
                String destMergePath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/testziliao/image_video.mp4";
                int encodeWidth = 0;
                int encodeHeight = 0;
                if (fBOImageWidth < fBOImageHeight) {
                    encodeWidth = 720;
                    encodeHeight = 1280;
                } else {
                    encodeWidth = 1280;
                    encodeHeight = 720;
                }
                Log.i("LivePusherPlayer", "fBOImageWidth: " + fBOImageWidth + ", fBOImageHeight: " + fBOImageHeight + ", fBOTextureId: " + fBOTextureId);
                mWlMediaEncoder_.initMediaEncoder(destMergePath, encodeWidth, encodeHeight, sampleRate, channels);
                mWlMediaEncoder_.startRecord();
                mWlImgVideoView_.setOnEncodeListener(new WLImgVideoView.OnEncodeListener() {
                    @Override
                    public void onEncode() {
                        mWlMediaEncoder_.requestRender();
                    }
                });
            }
            @Override
            public void onPcmData(byte[] pcmdata, int size, long clock) {
                if (mWlMediaEncoder_ != null) {
                    mWlMediaEncoder_.putPCMData(pcmdata, size);
                }
            }
        });
    }

    public void mergeVideo(View view) {
        Log.i("LivePusherPlayer", "mergeVideo start");
        mWlMusic_.setSource("/sdcard/testziliao/the_girl.m4a");
        mWlMusic_.prePared();
        mMergeBtn_.setText("正在合成");
    }

    private void _startImgs() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                for (int i = 1; i <= 257; i++) {
                    int imageId = getResources().getIdentifier("img_" + i, "drawable", "com.jiaquan.livepusher");
                    mWlImgVideoView_.setCurrentImg(imageId);
                    try {
                        Thread.sleep(50);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                if (mWlMusic_ != null) {
                    mWlMusic_.stop();
                }
                if (mWlMediaEncoder_ != null) {
                    mWlMediaEncoder_.stopRecord();
                }
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mMergeBtn_.setText("开始合成");
                    }
                });
                Log.i("LivePusherPlayer", "mergeVideo end");
            }
        }).start();
    }
}
