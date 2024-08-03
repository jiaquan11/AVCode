package com.jiaquan.openglesegl;

import androidx.appcompat.app.AppCompatActivity;

import android.opengl.GLES20;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class MainActivity extends AppCompatActivity {
    private SurfaceView mSurfaceView_ = null;
    private EglHelper mEglHelper_ = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mSurfaceView_ = findViewById(R.id.surfaceview);
        mSurfaceView_.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {

            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                new Thread() {
                    @Override
                    public void run() {
                        super.run();
                        mEglHelper_ = new EglHelper();
                        mEglHelper_.initEgl(holder.getSurface(), null);
                        while (true) {
                            GLES20.glViewport(0, 0, width, height);
                            GLES20.glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
                            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
                            mEglHelper_.swapBuffers();
                            try {
                                Thread.sleep(16);
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }.start();
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                mEglHelper_.destroyEgl();
            }
        });
    }
}