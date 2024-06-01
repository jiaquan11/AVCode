package com.jiaquan.mymusic;

import android.Manifest;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.widget.SeekBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.jiaquan.myplayer.muteenum.ChannelTypeEnum;
import com.jiaquan.myplayer.util.TimeInfoBean;
import com.jiaquan.myplayer.listener.OnCompleteListener;
import com.jiaquan.myplayer.listener.OnErrorListener;
import com.jiaquan.myplayer.listener.OnLoadListener;
import com.jiaquan.myplayer.listener.OnPauseResumeListener;
import com.jiaquan.myplayer.listener.OnPreparedListener;
import com.jiaquan.myplayer.listener.OnRecordTimeListener;
import com.jiaquan.myplayer.listener.OnTimeInfoListener;
import com.jiaquan.myplayer.listener.OnVolumeDBListener;
import com.jiaquan.myplayer.log.MyLog;
import com.jiaquan.myplayer.opengl.WLGLSurfaceView;
import com.jiaquan.myplayer.player.WLPlayer;
import com.jiaquan.myplayer.util.TimeUtil;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    private WLGLSurfaceView mWlglSurfaceView_ = null;
    private TextView mTvTime_ = null;
    private int mPosition_ = 0;
    private boolean mIsSeekBar_ = false;
    private SeekBar mSeekBarSeek_ = null;
    private TextView mTvVolume_ = null;
    private SeekBar mSeekBarVolume_ = null;
    private WLPlayer mWlPlayer_ = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // 要申请的权限
        String[] permissions = {Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.ACCESS_NETWORK_STATE, Manifest.permission.CHANGE_NETWORK_STATE};
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions, 321);
        }

        mWlglSurfaceView_ = findViewById(R.id.wlglsurfaceview);//播放GLSurfaceView控件
        mTvTime_ = findViewById(R.id.tv_time);//显示当前播放时间戳及播放总时长
        mSeekBarSeek_ = findViewById(R.id.seekbar_seek);//播放seek进度条
        mTvVolume_ = findViewById(R.id.tv_volume);//显示音量值
        mSeekBarVolume_ = findViewById(R.id.seekbar_volume);//调节音量的进度条
        mWlPlayer_ = new WLPlayer();
        mWlPlayer_.setWlglSurfaceView(mWlglSurfaceView_);
        mWlPlayer_.setVolume(80);//设置初始音量
        mTvVolume_.setText("音量: " + mWlPlayer_.getVolumePercent() + "%");//设置音量显示值
        mSeekBarVolume_.setProgress(mWlPlayer_.getVolumePercent());//设置音量进度条的初始值
        mWlPlayer_.setChannelType(ChannelTypeEnum.MUTE_LEFT);//设置声道控制
        mWlPlayer_.setSpeed(1.0f);//设置音频播放速度值
        mWlPlayer_.setPitch(1.0f);//设置音调值

        /**
         * 设置加载监听
         */
        mWlPlayer_.setOnLoadListener(new OnLoadListener() {
            @Override
            public void onLoad(boolean load) {
                if (load) {
                    MyLog.i("加载中");
                } else {
                    MyLog.i("播放中");
                }
            }
        });

        /**
         * 设置准备监听
         */
        mWlPlayer_.setOnPreparedListener(new OnPreparedListener() {
            @Override
            public void onPrepared() {
                MyLog.i("onPrepared");
                mWlPlayer_.start();
            }
        });

        /**
         * 设置暂停和恢复监听
         */
        mWlPlayer_.setOnPauseResumeListener(new OnPauseResumeListener() {
            @Override
            public void onPause(boolean pause) {
                if (pause) {
                    MyLog.i("暂停中");
                } else {
                    MyLog.i("恢复播放中");
                }
            }
        });

        /**
         * 设置进度条监听
         */
        mSeekBarSeek_.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {//进度条变化得到进度条的值
                if ((mWlPlayer_.getDuration() > 0) && mIsSeekBar_) {
                    mPosition_ = mWlPlayer_.getDuration() * (progress / 100);//实时更新进度条值，并转换为需要seek的点
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {//开始拖拽
                mIsSeekBar_ = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {//结束拖拽,进行seek
                mWlPlayer_.seek(mPosition_);
                mIsSeekBar_ = false;
            }
        });

        Handler handler = new Handler() {
            @Override
            public void handleMessage(@NonNull Message msg) {
                super.handleMessage(msg);
                if (msg.what == 1) {
                    if (!mIsSeekBar_) {//播放seek完成，更新seek进度条的位置及播放时间戳显示
                        TimeInfoBean timeInfoBean = (TimeInfoBean) msg.obj;
                        mTvTime_.setText(TimeUtil.secdsToDateFormat(timeInfoBean.getTotalTime(), timeInfoBean.getTotalTime()) + "/" +
                                         TimeUtil.secdsToDateFormat(timeInfoBean.getCurrentTime(), timeInfoBean.getTotalTime()));
                        mSeekBarSeek_.setProgress(timeInfoBean.getCurrentTime() * 100 / timeInfoBean.getTotalTime());
                    }
                }
            }
        };

        /**
         * 设置播放时间信息监听
         */
        mWlPlayer_.setOnTimeInfoListener(new OnTimeInfoListener() {
            @Override
            public void onTimeInfo(TimeInfoBean timeInfoBean) {
                Message message = Message.obtain();
                message.what = 1;
                message.obj = timeInfoBean;
                handler.sendMessage(message);//发送消息，放在Handle线程中进行更新处理时间显示
            }
        });

        /**
         * 设置音量调节监听
         */
        mSeekBarVolume_.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                mWlPlayer_.setVolume(progress);
                mTvVolume_.setText("音量: " + mWlPlayer_.getVolumePercent() + "%");
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

        /**
         * 设置播放完成监听
         */
        mWlPlayer_.setOnCompleteListener(new OnCompleteListener() {
            @Override
            public void onComplete() {
                MyLog.i("播放完成了");
            }
        });

        /**
         * 设置播放错误监听
         */
        mWlPlayer_.setOnErrorListener(new OnErrorListener() {
            @Override
            public void onError(int code, String msg) {
                MyLog.e("code is: " + code + " msg: " + msg);
            }
        });

        /**
         * 设置音频分贝值监听
         */
        mWlPlayer_.setOnVolumeDBListener(new OnVolumeDBListener() {
            @Override
            public void onDBValue(int db) {
//                MyLog.i("db is " + db);
            }
        });

        /**
         * 设置音频录制时间监听
         */
        mWlPlayer_.setOnRecordTimeListener(new OnRecordTimeListener() {
            @Override
            public void onAudioRecordTime(int recordTime) {
//                MyLog.i("record time is: " + recordTime);
            }
            @Override
            public void onVideoRecordTime(int recordTime) {
            }
        });
    }

    /**
     * 开始播放
     */
    public void start(View view) {
//        mWlPlayer_.prepared("http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3");
//        mWlPlayer_.prepared("/sdcard/testziliao/mydream.m4a");
//        mWlPlayer_.prepared("/sdcard/testziliao/fcrs.1080p.mp4");
//        mWlPlayer_.prepared("/sdcard/testziliao/biterate9.mp4");
//        mWlPlayer_.prepared("/sdcard/testziliao/biterate9noB.mp4");
//        wmWlPlayer_.prepared("/sdcard/testziliao/chuqiaozhuan1.mp4");
//        mWlPlayer_.prepared("/sdcard/testziliao/first-love-wangxinling.ape");
//        mWlPlayer_.prepared("http://ngcdn004.cnr.cn/live/dszs/index.m3u8");
//        mWlPlayer_.prepared("/sdcard/testziliao/yongqi-liangjingru.m4a");
        mWlPlayer_.prepared("/sdcard/testziliao/hanleiVideo.mp4");
    }

    /**
     * 暂停播放
     */
    public void pause(View view) {
        mWlPlayer_.pause();
    }

    /**
     * 恢复播放
     */
    public void resume(View view) {
        mWlPlayer_.resume();
    }

    /**
     * 停止播放
     */
    public void stop(View view) {
        mWlPlayer_.stop();
    }

    /**
     * 固定seek 每次20s
     */
    public void seek(View view) {
        mWlPlayer_.seek(20);
    }

    /**
     * 切换下一个播放源
     */
    public void next(View view) {
//        wlPlayer.playNext("http://ngcdn004.cnr.cn/live/dszs/index.m3u8");
//        wlPlayer.playNext("/sdcard/testziliao/first-love-wangxinling.ape");
//        wlPlayer.playNext("/sdcard/testziliao/fcrs.1080p.mp4");
        mWlPlayer_.playNext("/sdcard/testziliao/建国大业.mpg");
    }

    /**
     * 左声道
     */
    public void left(View view) {
        mWlPlayer_.setChannelType(ChannelTypeEnum.MUTE_LEFT);
    }

    /**
     * 右声道
     */
    public void right(View view) {
        mWlPlayer_.setChannelType(ChannelTypeEnum.MUTE_RIGHT);
    }

    /**
     * 立体声
     */
    public void center(View view) {
        mWlPlayer_.setChannelType(ChannelTypeEnum.MUTE_CENTER);
    }

    /**
     * 变速不变调
     */
    public void speed(View view) {
        mWlPlayer_.setPitch(1.0f);
        mWlPlayer_.setSpeed(1.5f);
    }

    /**
     * 变调不变速
     */
    public void pitch(View view) {
        mWlPlayer_.setPitch(1.5f);
        mWlPlayer_.setSpeed(1.0f);
    }

    /**
     * 变速变调
     */
    public void speedPitch(View view) {
        mWlPlayer_.setPitch(1.5f);
        mWlPlayer_.setSpeed(1.5f);
    }

    /**
     * 正常音调和速度
     */
    public void normalSpeedPitch(View view) {
        mWlPlayer_.setPitch(1.0f);
        mWlPlayer_.setSpeed(1.0f);
    }

    /**
     * 开始录音
     */
    public void startAudioRecord(View view) {
        mWlPlayer_.startAudioRecord(new File("/sdcard/testziliao/testplayer1.aac"));
    }

    /**
     * 暂停录音
     */
    public void pauseAudioRecord(View view) {
        mWlPlayer_.pauseAudioRecord();
    }

    /**
     * 恢复录音
     */
    public void resumeAudioRecord(View view) {
        mWlPlayer_.resumeAudioRecord();
    }

    /**
     * 停止录音
     */
    public void stopAudioRecord(View view) {
        mWlPlayer_.stopAudioRecord();
    }
}