package com.jiaquan.xplay;

import android.Manifest;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.SeekBar;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity implements Runnable, SeekBar.OnSeekBarChangeListener {
    private static final String TAG = MainActivity.class.getSimpleName();

    static {
        System.loadLibrary("native-lib");
    }

    private Button bt = null;
    private SeekBar seek = null;
    private Thread thread = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // 要申请的权限
        String[] permissions = {Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE
                , Manifest.permission.ACCESS_NETWORK_STATE, Manifest.permission.CHANGE_NETWORK_STATE};

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions, 321);
        }

        //去掉标题栏
        supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
        //全屏，隐藏状态
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        //屏幕为横屏
//        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);//横屏
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);//竖屏

        setContentView(R.layout.activity_main);

        bt = findViewById(R.id.open_button);
        seek = findViewById(R.id.aplayseek);
        seek.setMax(1000);
        seek.setOnSeekBarChangeListener(this);

        bt.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.e(TAG, "open button click");
                Intent intent = new Intent();
                intent.setClass(MainActivity.this, OpenUrl.class);//跳转页面
                startActivity(intent);
            }
        });

        //播放进度线程
        thread = new Thread(this);
        thread.start();//开启线程
    }

    //播放进度显示
    @Override
    public void run() {//线程函数，用于更新播放进度
        for (; ;) {
            seek.setProgress((int) (PlayPos() * 1000));//根据解码时间戳去更新播放进度
            try {
                Thread.sleep(40);//40毫秒进度条进行刷新一遍
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int i, boolean b) {

    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {//拖拽进度条进行seek
        Seek((double) seekBar.getProgress() / (double) seekBar.getMax());//seek参数转换为0-1之间
    }

    public native double PlayPos();

    public native void Seek(double pos);
}
