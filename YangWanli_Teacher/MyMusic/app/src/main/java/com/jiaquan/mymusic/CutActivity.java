package com.jiaquan.mymusic;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.jiaquan.myplayer.listener.OnPcmInfoListener;
import com.jiaquan.myplayer.listener.OnPreparedListener;
import com.jiaquan.myplayer.listener.OnTimeInfoListener;
import com.jiaquan.myplayer.log.MyLog;
import com.jiaquan.myplayer.player.WLPlayer;
import com.jiaquan.myplayer.util.TimeInfoBean;

/**
 * 音频裁剪播放(设置开始时间和结束时间)
 */
public class CutActivity extends AppCompatActivity {
    private WLPlayer mWlPlayer_ = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_cutaudio);

        mWlPlayer_ = new WLPlayer();
        /**
         * 设置准备监听
         */
        mWlPlayer_.setOnPreparedListener(new OnPreparedListener() {
            @Override
            public void onPrepared() {
                mWlPlayer_.cutAudioPlay(20, 40, true);
            }
        });

        /**
         * 设置时间信息监听
         */
        mWlPlayer_.setOnTimeInfoListener(new OnTimeInfoListener() {
            @Override
            public void onTimeInfo(TimeInfoBean timeInfoBean) {
                MyLog.i(timeInfoBean.toString());
            }
        });

        /**
         * 设置pcm数据监听
         */
        mWlPlayer_.setOnPcmInfoListener(new OnPcmInfoListener() {
            @Override
            public void onPcmRate(int samplerate, int bit, int channels) {
                MyLog.i("PcmInfo samplerate: " + samplerate + " bit:" + bit + " channels:" + channels);
            }

            @Override
            public void onPcmInfo(byte[] buffer, int buffersize) {
                MyLog.i("PcmInfo bufferSize: " + buffersize);
            }
        });
    }

    /**
     * 音频裁剪播放
     * @param view
     */
    public void cutAudio(View view) {
//        mWlPlayer_.prepared("/sdcard/testziliao/first-love-wangxinling.ape");
//        mWlPlayer_.prepared("/sdcard/testziliao/mydream.m4a");
        mWlPlayer_.prepare("/sdcard/testziliao/the_girl.m4a");
    }
}
