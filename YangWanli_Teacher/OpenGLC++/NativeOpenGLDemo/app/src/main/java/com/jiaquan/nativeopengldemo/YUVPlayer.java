package com.jiaquan.nativeopengldemo;

import android.Manifest;
import android.os.Build;
import android.os.Build.VERSION_CODES;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import com.jiaquan.opengl.NativeOpengl;
import com.jiaquan.opengl.WlSurfaceView;

import java.io.File;
import java.io.FileInputStream;

@RequiresApi(api = VERSION_CODES.M)
public class YUVPlayer extends AppCompatActivity {
    private WlSurfaceView mWLSurfaceView_ = null;
    private NativeOpengl mNativeOpengl_ = null;
    private boolean mIsExit_ = false;
    private FileInputStream mFileInputStream_ = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_yuvplayer);

        // 要申请的权限
        String[] permissions = {Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE,
                Manifest.permission.ACCESS_NETWORK_STATE, Manifest.permission.CHANGE_NETWORK_STATE};
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(permissions, 321);
        }

        mWLSurfaceView_ = findViewById(R.id.wlSurfaceview);
        mNativeOpengl_ = new NativeOpengl();
        mWLSurfaceView_.setNativeOpengl(mNativeOpengl_);
    }

    public void play(View view) {
        if (!mIsExit_) {
            mIsExit_ = false;

            //创建一个子线程进行yuv数据的读取和渲染
            new Thread(new Runnable() {
                @Override
                public void run() {
                    int w = 720;
                    int h = 1280;

                    try {
                        mFileInputStream_ = new FileInputStream(new File("/sdcard/testziliao/biterate9.yuv"));

                        byte[] y = new byte[w * h];
                        byte[] u = new byte[w * h / 4];
                        byte[] v = new byte[w * h / 4];

                        while (true) {
                            if (mIsExit_) {
                                break;
                            }

                            int ysize = mFileInputStream_.read(y);
                            int usize = mFileInputStream_.read(u);
                            int vsize = mFileInputStream_.read(v);
                            if ((ysize > 0) && (usize > 0) && (vsize > 0)) {
                                mNativeOpengl_.nativeSetYuvData(w, h, y, u, v);
                                Thread.sleep(40);
                            } else {
                                mIsExit_ = true;
                            }
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            }).start();
        }
    }

    public void stop(View view) {
        mIsExit_ = true;
    }
}
