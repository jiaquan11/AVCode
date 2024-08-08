package com.jiaquan.livepusher.camera;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.WindowManager;

import com.jiaquan.livepusher.egl.WLEGLSurfaceView;

public class WLCameraView extends WLEGLSurfaceView {
    private WLCameraRender mWlCameraRender_ = null;
    private WLCamera mWlCamera_ = null;
    private int mCameraId_ = Camera.CameraInfo.CAMERA_FACING_BACK;
    private int mTextureId_ = -1;
    private Context mContext_;

    public WLCameraView(Context context) {
        this(context, null);
    }

    public WLCameraView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public WLCameraView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        Log.i("LivePusherPlayer", "WLCameraView init");
        mContext_ = context;
        mWlCamera_ = new WLCamera();
        mWlCameraRender_ = new WLCameraRender(context);
        setRender(mWlCameraRender_);
        setRenderMode(WLEGLSurfaceView.RENDERMODE_WHEN_DIRTY);
        mWlCamera_.openCamera(mCameraId_);//默认打开后置摄像头
        mWlCameraRender_.setOnSurfaceCreateListener(new WLCameraRender.OnSurfaceCreateListener() {
            @Override
            public void onSurfaceCreate(SurfaceTexture surfaceTexture, int textureid, int surfaceWidth, int surfaceHeight) {
                Log.i("LivePusherPlayer", "WLCameraView onSurfaceCreate");
                previewAngle(context);
                mWlCamera_.startPreview(surfaceTexture, surfaceWidth, surfaceHeight);
                mTextureId_ = textureid;
            }
        });
        mWlCameraRender_.setOnRenderListener(new WLCameraRender.OnRenderListener() {
            @Override
            public void onRender() {
                requestRender();
            }
        });
        Log.i("LivePusherPlayer", "WLCameraView init end");
    }

    public void onDestroy() {
        if (mWlCamera_ != null) {
            mWlCamera_.stopPreview();
            mWlCamera_.closeCamera();
        }
        if (mWlCameraRender_ != null) {
            mWlCameraRender_.onDestory();
        }
    }

    public void previewAngle(Context context) {
        mWlCameraRender_.resetMatrix();
        int angle = ((WindowManager) context.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay().getRotation();
        Log.i("LivePusherPlayer", "previewAngle in, angle: " + angle);
        switch (angle) {
            case Surface.ROTATION_0:
                if (mCameraId_ == Camera.CameraInfo.CAMERA_FACING_BACK) {
                    mWlCameraRender_.setAngle(90, 0, 0, 1);//先旋转Z轴
                    mWlCameraRender_.setAngle(180, 1, 0, 0);//再旋转X轴
                } else {
                    mWlCameraRender_.setAngle(90, 0, 0, 1);
                }
                break;
            case Surface.ROTATION_90:
                if (mCameraId_ == Camera.CameraInfo.CAMERA_FACING_BACK) {
                    mWlCameraRender_.setAngle(180, 0, 0, 1);
                    mWlCameraRender_.setAngle(180, 0, 1, 0);
                } else {
                    mWlCameraRender_.setAngle(90, 0, 0, 1);
                }
                break;
            case Surface.ROTATION_180:
                if (mCameraId_ == Camera.CameraInfo.CAMERA_FACING_BACK) {
                    mWlCameraRender_.setAngle(90, 0, 0, 1);
                    mWlCameraRender_.setAngle(180, 0, 1, 0);
                } else {
                    mWlCameraRender_.setAngle(-90, 0, 0, 1);
                }
                break;
            case Surface.ROTATION_270:
                if (mCameraId_ == Camera.CameraInfo.CAMERA_FACING_BACK) {
                    mWlCameraRender_.setAngle(180, 0, 1, 0);
                } else {
                    mWlCameraRender_.setAngle(0, 0, 0, 1);
                }
                break;
        }
        Log.i("LivePusherPlayer", "previewAngle end");
    }

    public int getTextureId() {
        return mTextureId_;
    }

    public void switchCamera() {
        if (mCameraId_ == Camera.CameraInfo.CAMERA_FACING_BACK) {
            mCameraId_ = Camera.CameraInfo.CAMERA_FACING_FRONT;
        } else {
            mCameraId_ = Camera.CameraInfo.CAMERA_FACING_BACK;
        }
        mWlCamera_.openCamera(mCameraId_);
        previewAngle(mContext_);
        mWlCamera_.startPreview(mWlCamera_.getSurfaceTexture(), mWlCamera_.getSurfaceWidth(), mWlCamera_.getSurfaceHeight());
    }
}
