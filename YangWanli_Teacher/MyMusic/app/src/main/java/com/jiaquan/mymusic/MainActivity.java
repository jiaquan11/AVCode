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
import com.jiaquan.myplayer.muteenum.MuteEnum;
import com.jiaquan.myplayer.opengl.WLGLSurfaceView;
import com.jiaquan.myplayer.player.WLPlayer;
import com.jiaquan.myplayer.util.TimeUtil;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    private TextView tv_time = null;
    private TextView tv_volume = null;
    private SeekBar seekBarSeek = null;
    private int position = 0;
    private boolean isSeekBar = false;
    private SeekBar seekBarVolume = null;

    private WLPlayer wlPlayer = null;
    private WLGLSurfaceView wlglSurfaceView = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        wlglSurfaceView = findViewById(R.id.wlglsurfaceview);//主页面获取GLSurfaceView控件

        tv_time = findViewById(R.id.tv_time);//当前播放时间戳及总时间
        seekBarSeek = findViewById(R.id.seekbar_seek);//seek进度条
        tv_volume = findViewById(R.id.tv_volume);//音量值
        seekBarVolume = findViewById(R.id.seekbar_volume);//调节音量的进度条

        // 要申请的权限
        String[] permissions = {Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.ACCESS_NETWORK_STATE, Manifest.permission.CHANGE_NETWORK_STATE};
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions, 321);
        }

        wlPlayer = new WLPlayer();
        wlPlayer.setWlglSurfaceView(wlglSurfaceView);

        //初始化设置
        wlPlayer.setVolume(80);//设置初始音量
        tv_volume.setText("音量: " + wlPlayer.getVolumePercent() + "%");//设置当前音量值显示的文本
        seekBarVolume.setProgress(wlPlayer.getVolumePercent());//设置音量调节控件的进度,就是用当前音量值设置更新到进度条上面
        wlPlayer.setMute(MuteEnum.MUTE_LEFT);//设置声道控制
        wlPlayer.setPitch(1.0f);//设置音调值
        wlPlayer.setSpeed(1.0f);//设置音频播放速度值

        //Handler消息处理
        Handler handler = new Handler() {
            @Override
            public void handleMessage(@NonNull Message msg) {//重写类方法 处理Handler的消息
                super.handleMessage(msg);
                if (msg.what == 1) {
                    if (!isSeekBar) {//拖拽结束后，更新seek进度条的位置及播放时间戳显示
                        TimeInfoBean timeInfoBean = (TimeInfoBean) msg.obj;
                        tv_time.setText(TimeUtil.secdsToDateFormat(timeInfoBean.getTotalTime(), timeInfoBean.getTotalTime()) + "/" + TimeUtil.secdsToDateFormat(timeInfoBean.getCurrentTime(), timeInfoBean.getTotalTime()));
                        seekBarSeek.setProgress(timeInfoBean.getCurrentTime() * 100 / timeInfoBean.getTotalTime());
                    }
                }
            }
        };

        /*
        * 监听回调设置
        * */
        //资源准备回调，准备好后会进行回调，然后就可以开始播放
        wlPlayer.setOnPreparedListener(new OnPreparedListener() {
            @Override
            public void onPrepared() {
                MyLog.i("onPrepared");
                wlPlayer.start();//开始播放
            }
        });

        //加载回调
        wlPlayer.setOnLoadListener(new OnLoadListener() {
            @Override
            public void onLoad(boolean load) {
                if (load) {
                    MyLog.i("加载中");
                } else {
                    MyLog.i("播放中");
                }
            }
        });

        //暂停和播放的回调
        wlPlayer.setOnPauseResumeListener(new OnPauseResumeListener() {
            @Override
            public void onPause(boolean pause) {
                if (pause) {
                    MyLog.i("暂停中");
                } else {
                    MyLog.i("恢复播放中");
                }
            }
        });

        //设置seek进度条监听
        seekBarSeek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {//进度条变化得到进度条的值
                if ((wlPlayer.getDuration() > 0) && isSeekBar) {
                    position = wlPlayer.getDuration() * (progress / 100);//实时更新进度条值，并转换为需要seek的点
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {//开始拖拽
                isSeekBar = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {//结束拖拽,进行seek
                wlPlayer.seek(position);
                isSeekBar = false;
            }
        });

        //回调时间戳信息，用于播放进度的自动显示
        wlPlayer.setOnTimeInfoListener(new OnTimeInfoListener() {
            @Override
            public void onTimeInfo(TimeInfoBean timeInfoBean) {
//                MyLog.i(timeInfoBean.toString());
                Message message = Message.obtain();
                message.what = 1;
                message.obj = timeInfoBean;
                handler.sendMessage(message);//发送消息，放在Handle线程中进行更新处理时间显示
            }
        });

        //音量进度条操作
        seekBarVolume.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                wlPlayer.setVolume(progress);
                tv_volume.setText("音量: " + wlPlayer.getVolumePercent() + "%");
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

        //回调播放完成状态
        wlPlayer.setOnCompleteListener(new OnCompleteListener() {
            @Override
            public void onComplete() {
                MyLog.i("播放完成了");
            }
        });

        //回调错误信息
        wlPlayer.setOnErrorListener(new OnErrorListener() {
            @Override
            public void onError(int code, String msg) {
                MyLog.i("code is: " + code + " msg: " + msg);
            }
        });

        //回调音量分贝值
        wlPlayer.setOnVolumeDBListener(new OnVolumeDBListener() {
            @Override
            public void onDBValue(int db) {
//                MyLog.i("db is " + db);
            }
        });

        //回调录制时间戳
        wlPlayer.setOnRecordTimeListener(new OnRecordTimeListener() {
            @Override
            public void onRecordTime(int recordTime) {
//                MyLog.i("record time is: " + recordTime);
            }
        });
    }

    //页面控件的按钮响应函数
    //开始播放
    public void begin(View view) {
//        wlPlayer.setSource("http://mpge.5nd.com/2015/2015-11-26/69708/1.mp3");
//        wlPlayer.setSource("/sdcard/testziliao/mydream.m4a");
//        wlPlayer.setSource("/sdcard/testziliao/fcrs.1080p.mp4");
//        wlPlayer.setSource("/sdcard/testziliao/biterate9.mp4");
//        wlPlayer.setSource("/sdcard/testziliao/biterate9noB.mp4");
//        wlPlayer.setSource("/sdcard/testziliao/chuqiaozhuan1.mp4");
        wlPlayer.setSource("/sdcard/testziliao/hanleiVideo.mp4");

//        wlPlayer.setSource("/sdcard/testziliao/first-love-wangxinling.ape");
//        wlPlayer.setSource("http://ngcdn004.cnr.cn/live/dszs/index.m3u8");
//        wlPlayer.setSource("/sdcard/testziliao/yongqi-liangjingru.m4a");
        wlPlayer.prepared();
    }

    //暂停播放
    public void pause(View view) {
        wlPlayer.pause();
    }

    //恢复播放
    public void resume(View view) {
        wlPlayer.resume();
    }

    //停止播放
    public void stop(View view) {
        wlPlayer.stop();
    }

    //seek操作(按钮点击)，每次seek 20s
    public void seek(View view) {
        wlPlayer.seek(20);
    }

    //切换下一个播放资源
    public void next(View view) {
//        wlPlayer.playNext("http://ngcdn004.cnr.cn/live/dszs/index.m3u8");
//        wlPlayer.playNext("/sdcard/testziliao/first-love-wangxinling.ape");
//        wlPlayer.playNext("/sdcard/testziliao/fcrs.1080p.mp4");
        wlPlayer.playNext("/sdcard/testziliao/建国大业.mpg");
    }

    //左声道
    public void left(View view) {
        wlPlayer.setMute(MuteEnum.MUTE_LEFT);
    }

    //右声道
    public void right(View view) {
        wlPlayer.setMute(MuteEnum.MUTE_RIGHT);
    }

    //立体声
    public void center(View view) {
        wlPlayer.setMute(MuteEnum.MUTE_CENTER);
    }

    //变速不变调
    public void speed(View view) {
        wlPlayer.setPitch(1.0f);
        wlPlayer.setSpeed(1.5f);
    }

    //变调不变速
    public void pitch(View view) {
        wlPlayer.setPitch(1.5f);
        wlPlayer.setSpeed(1.0f);
    }

    //变速又变调
    public void speedpitch(View view) {
        wlPlayer.setPitch(1.5f);
        wlPlayer.setSpeed(1.5f);
    }

    //正常音调和速度
    public void normalspeedpitch(View view) {
        wlPlayer.setPitch(1.0f);
        wlPlayer.setSpeed(1.0f);
    }

    //开始音频录制,指定了录制输出文件
    public void start_record(View view) {
        wlPlayer.startRecord(new File("/sdcard/testziliao/testplayer1.aac"));
    }

    //暂停音频录制
    public void pause_record(View view) {
        wlPlayer.pauseRecord();
    }

    //恢复音频录制
    public void resume_record(View view) {
        wlPlayer.resumeRecord();
    }

    //停止音频录制
    public void stop_record(View view) {
        wlPlayer.stopRecord();
    }
}