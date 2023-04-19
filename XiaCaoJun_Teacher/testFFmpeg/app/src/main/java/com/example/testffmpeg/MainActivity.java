package com.example.testffmpeg;

import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //去掉标题栏
        supportRequestWindowFeature(Window.FEATURE_NO_TITLE);
        //全屏，隐藏状态
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        //屏幕为横屏
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);

        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);

        String url = "/sdcard/testziliao/biterate9.mp4";
        //Open(url, this);

        tv.setText(stringFromJNI());
    }

    //native method
    public native String stringFromJNI();

    public native boolean Open(String url, Object handle);
}
