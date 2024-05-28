package com.jiaquan.mymusic;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.jiaquan.myplayer.util.TimeInfoBean;
import com.jiaquan.myplayer.listener.OnPcmInfoListener;
import com.jiaquan.myplayer.listener.OnPreparedListener;
import com.jiaquan.myplayer.listener.OnTimeInfoListener;
import com.jiaquan.myplayer.log.MyLog;
import com.jiaquan.myplayer.player.WLPlayer;

public class CutActivity extends AppCompatActivity {
    private WLPlayer wlPlayer = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_cutaudio);

        wlPlayer = new WLPlayer();
        //设置相关的监听回调
        //资源准备好监听
        wlPlayer.setOnPreparedListener(new OnPreparedListener() {
            @Override
            public void onPrepared() {
                wlPlayer.cutAudioPlay(20, 40, true);//开始裁剪，并回调pcm数据
            }
        });

        //播放时间戳信息监听
        wlPlayer.setOnTimeInfoListener(new OnTimeInfoListener() {
            @Override
            public void onTimeInfo(TimeInfoBean timeInfoBean) {
                MyLog.i(timeInfoBean.toString());
            }
        });

        //音频解码pcm数据监听
        wlPlayer.setOnPcmInfoListener(new OnPcmInfoListener() {
            @Override
            public void onPcmInfo(byte[] buffer, int buffersize) {
                MyLog.i("PcmInfo bufferSize: " + buffersize);
            }

            @Override
            public void onPcmRate(int samplerate, int bit, int channels) {
                MyLog.i("PcmInfo samplerate: " + samplerate + " bit:" + bit + " channels:" + channels);
            }
        });
    }

    /**
     * 音频裁剪
     * @param view
     */
    public void cutAudio(View view) {
//        wlPlayer.prepared("/sdcard/testziliao/first-love-wangxinling.ape");
        wlPlayer.prepared("/sdcard/testziliao/mydream.m4a");
    }
}
