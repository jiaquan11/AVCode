package com.jiaquan.livepusher.camera;

import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.util.Log;

import java.io.IOException;
import java.util.List;

public class WLCamera {
    private SurfaceTexture mSurfaceTexture_ = null;
    private Camera mCamera_ = null;
    private int mScreenWidth_ = 0;
    private int mScreenHeight_ = 0;

    public WLCamera() {

    }

    public void openCamera(int cameraId) {
        Log.i("LivePusherPlayer", "openCamera cameraId:" + cameraId);
        stopPreview();
        closeCamera();
        mCamera_ = Camera.open(cameraId);
        if (mCamera_ == null) {
            Log.e("LivePusherPlayer", "open camera failed");
        }
    }
    public void startPreview(SurfaceTexture surfaceTexture, int surfaceWidth, int surfaceHeight) {
        Log.i("LivePusherPlayer", "startPreview, mCamera_:" + mCamera_ + " surfaceWidth:" + surfaceWidth + " surfaceHeight:" + surfaceHeight);
        mSurfaceTexture_ = surfaceTexture;
        mScreenWidth_ = surfaceWidth;
        mScreenHeight_ = surfaceHeight;
        if (mCamera_ != null) {
            try {
                mCamera_.setPreviewTexture(surfaceTexture);
                Camera.Parameters parameters = mCamera_.getParameters();
                parameters.setFlashMode("off");
                parameters.setPreviewFormat(ImageFormat.NV21);
                Camera.Size size = _getFitSize(parameters.getSupportedPictureSizes(), surfaceWidth, surfaceHeight);
                // 设置拍摄照片的分辨率
                parameters.setPictureSize(size.width, size.height);
                Log.i("LivePusherPlayer", "setPictureSize width: " + size.width + " height: " + size.height);
                size = _getFitSize(parameters.getSupportedPreviewSizes(), surfaceWidth, surfaceHeight);
                // 设置实时预览的分辨率
                parameters.setPreviewSize(size.width, size.height);
                Log.i("LivePusherPlayer", "setPreviewSize width: " + size.width + " height: " + size.height);
                mCamera_.setParameters(parameters);
                mCamera_.startPreview();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public void stopPreview() {
        Log.i("LivePusherPlayer", "stopPreview, mCamera_:" + mCamera_);
        if (mCamera_ != null) {
            mCamera_.stopPreview();
        }
    }

    public void closeCamera() {
        if (mCamera_ != null) {
            mCamera_.release();
            mCamera_ = null;
        }
    }

    public SurfaceTexture getSurfaceTexture() {
        return mSurfaceTexture_;
    }

    public int getSurfaceWidth() {
        return mScreenWidth_;
    }

    public int getSurfaceHeight() {
        return mScreenHeight_;
    }

    /**
     * 从给定的一组相机支持的预览尺寸中选择一个与屏幕尺寸比例相符的大小，以确保预览画面不会失真或变形。
     * 如果找不到一个与屏幕尺寸比例完全匹配的预览尺寸，那么就返回列表中的第一个尺寸作为默认值
     */
    private Camera.Size _getFitSize(List<Camera.Size> sizes, int surfaceWidth, int surfaceHeight) {
        Log.i("LivePusherPlayer", "_getFitSize surfaceWidth: " + surfaceWidth + " surfaceHeight: " + surfaceHeight);
        if (surfaceWidth < surfaceHeight) {
            int t = surfaceHeight;
            surfaceHeight = surfaceWidth;
            surfaceWidth = t;
        }
        for (Camera.Size size : sizes) {
            if ((1.0f * size.width / size.height) == (1.0f * surfaceWidth / surfaceHeight)) {
                return size;
            }
        }
        return sizes.get(0);
    }
}
