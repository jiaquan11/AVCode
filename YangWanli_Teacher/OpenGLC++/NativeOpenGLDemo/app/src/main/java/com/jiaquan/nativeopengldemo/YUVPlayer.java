package com.jiaquan.nativeopengldemo;

import android.Manifest;
import android.os.Build;
import android.os.Build.VERSION_CODES;
import android.os.Bundle;
import android.util.Log;
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
    private static final String TAG = YUVPlayer.class.getSimpleName();

    private WlSurfaceView mWLSurfaceView_ = null;
    private NativeOpengl mNativeOpengl_ = null;
    private boolean mIsExit_ = true;
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
        mWLSurfaceView_.setOnSurfaceListener(new NativeOpengl.OnSurfaceListener() {
            @Override
            public void init() {
                mIsExit_ = false;
            }
        });
    }

    public void play(View view) {
        Log.i(TAG, "play enter, is exit: " + mIsExit_);
        if (!mIsExit_) {
            /**
             * 读取yuv数据
             */
            new Thread(new Runnable() {
                @Override
                public void run() {
                    int yuvWidth = 720;
                    int yuvHeight = 1280;
                    try {
                        mFileInputStream_ = new FileInputStream(new File("/sdcard/testziliao/biterate9.yuv"));
                        byte[] ybuf = new byte[yuvWidth * yuvHeight];
                        byte[] ubuf = new byte[yuvWidth * yuvHeight / 4];
                        byte[] vbuf = new byte[yuvWidth * yuvHeight / 4];
                        while (true) {
                            if (mIsExit_) {
                                break;
                            }
                            int ySize = mFileInputStream_.read(ybuf);
                            int uSize = mFileInputStream_.read(ubuf);
                            int vSize = mFileInputStream_.read(vbuf);
                            if ((ySize > 0) && (uSize > 0) && (vSize > 0)) {
                                mNativeOpengl_.nativeSetYuvData(yuvWidth, yuvHeight, ybuf, ubuf, vbuf);
                                Thread.sleep(40);
                            } else {
                                Log.i(TAG, "read end");
                                mIsExit_ = true;
                            }
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                    }

                    if (mFileInputStream_ != null) {
                        try {
                            mFileInputStream_.close();
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                    mIsExit_ = false;
                    Log.i(TAG, "play end");
                }
            }).start();
        }
    }

    public void stop(View view) {
        mIsExit_ = true;
    }
}
