package com.jiaquan.livepusher;

import android.os.Bundle;
import android.util.Log;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.jiaquan.livepusher.yuv.WLYuvView;

import java.io.File;
import java.io.FileInputStream;

public class YuvActivity extends AppCompatActivity {
    private WLYuvView mWlYuvView_ = null;
    private FileInputStream mFis_ =  null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_yuv);
        mWlYuvView_ = findViewById(R.id.yuvView);
    }

    public void startPlay(View view) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    int yuvWidth = 640;
                    int yuvHeight = 360;
                    mFis_ = new FileInputStream(new File("/sdcard/testziliao/sintel_640_360.yuv"));//biterate9.yuv sintel_640_360.yuv
                    byte[] ydata = new byte[yuvWidth * yuvHeight];
                    byte[] udata = new byte[yuvWidth * yuvHeight / 4];
                    byte[] vdata = new byte[yuvWidth * yuvHeight / 4];
                    while (true) {
                        int ySize = mFis_.read(ydata);
                        int uSize = mFis_.read(udata);
                        int vSize = mFis_.read(vdata);
                        if ((ySize > 0) && (uSize > 0) && (vSize > 0)) {
                            mWlYuvView_.setYuvData(yuvWidth, yuvHeight, ydata, udata, vdata);
                            Thread.sleep(40);
                        } else {
                            if (mFis_ != null) {
                                mFis_.close();
                                mFis_ = null;
                            }
                            Log.i("YuvActivity", "完成yuv播放!");
                            break;
                        }
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }).start();
    }
}
